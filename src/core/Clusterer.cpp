/**
 * Clusterer implementation.
 * Groups similar faces into clusters using agglomerative clustering.
 * See docs/specs/003-face-clusterer.md for specification.
 */

#include "Clusterer.h"
#include "../services/Database.h"
#include "../services/FaceService.h"
#include <algorithm>
#include <limits>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <set>

namespace facefling {

// Get current timestamp as ISO string
static std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

class Clusterer::Impl {
public:
    std::shared_ptr<IDatabase> database;
    std::shared_ptr<FaceService> face_service;
    Config config;
    
    // Compute centroid (average) of multiple embeddings
    std::vector<float> compute_centroid(const std::vector<FaceEmbedding>& embeddings) {
        if (embeddings.empty()) return {};
        
        const size_t dims = 128;
        std::vector<float> centroid(dims, 0.0f);
        
        for (const auto& emb : embeddings) {
            if (emb.size() != dims) continue;
            for (size_t i = 0; i < dims; ++i) {
                centroid[i] += emb[i];
            }
        }
        
        float n = static_cast<float>(embeddings.size());
        for (size_t i = 0; i < dims; ++i) {
            centroid[i] /= n;
        }
        
        return centroid;
    }
    
    // Update cluster's centroid after faces have been modified
    void update_cluster_centroid(int64_t cluster_id) {
        std::vector<Face> faces = database->get_faces_for_cluster(cluster_id);
        
        std::vector<FaceEmbedding> embeddings;
        embeddings.reserve(faces.size());
        for (const auto& f : faces) {
            if (f.has_embedding()) {
                embeddings.push_back(f.embedding);
            }
        }
        
        auto centroid = compute_centroid(embeddings);
        if (!centroid.empty()) {
            database->update_cluster_centroid(cluster_id, centroid);
        }
    }
    
    // Find the cluster whose centroid is nearest to given embedding
    std::optional<int64_t> find_nearest_cluster(
        const FaceEmbedding& embedding,
        const std::vector<Cluster>& clusters)
    {
        if (clusters.empty()) return std::nullopt;
        
        int64_t best_id = 0;
        float best_dist = std::numeric_limits<float>::max();
        
        for (const auto& cluster : clusters) {
            if (cluster.centroid.empty()) continue;
            
            float dist = FaceService::embedding_distance(embedding, cluster.centroid);
            if (dist < best_dist) {
                best_dist = dist;
                best_id = cluster.id;
            }
        }
        
        // Only return if within threshold
        if (best_dist <= config.distance_threshold) {
            return best_id;
        }
        
        return std::nullopt;
    }
};

Clusterer::Clusterer(
    std::shared_ptr<IDatabase> database,
    std::shared_ptr<FaceService> face_service,
    const Config& config)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->database = database;
    m_impl->face_service = face_service;
    m_impl->config = config;
}

Clusterer::~Clusterer() = default;

void Clusterer::cluster_all(ProgressCallback progress)
{
    // Get all faces with embeddings that aren't already clustered
    std::vector<Face> faces = m_impl->database->get_all_faces_with_embeddings();
    
    if (faces.empty()) {
        std::cout << "[Clusterer] No faces to cluster" << std::endl;
        return;
    }
    
    std::cout << "[Clusterer] Clustering " << faces.size() << " faces..." << std::endl;
    
    // Structure to track cluster building
    struct WorkingCluster {
        std::vector<int64_t> face_ids;
        FaceEmbedding centroid;
    };
    
    // Start with each face in its own cluster
    std::vector<WorkingCluster> clusters;
    clusters.reserve(faces.size());
    
    for (const auto& face : faces) {
        WorkingCluster wc;
        wc.face_ids.push_back(face.id);
        wc.centroid = face.embedding;
        clusters.push_back(std::move(wc));
    }
    
    const int total_faces = static_cast<int>(faces.size());
    int merges_done = 0;
    
    // Agglomerative clustering: iteratively merge closest clusters
    while (clusters.size() > 1) {
        float min_dist = std::numeric_limits<float>::max();
        size_t merge_i = 0, merge_j = 0;
        
        // Find closest pair of clusters
        for (size_t i = 0; i < clusters.size(); ++i) {
            for (size_t j = i + 1; j < clusters.size(); ++j) {
                float dist = FaceService::embedding_distance(
                    clusters[i].centroid, 
                    clusters[j].centroid
                );
                if (dist < min_dist) {
                    min_dist = dist;
                    merge_i = i;
                    merge_j = j;
                }
            }
        }
        
        // Stop if no clusters are close enough
        if (min_dist > m_impl->config.distance_threshold) {
            break;
        }
        
        // Merge cluster j into cluster i
        clusters[merge_i].face_ids.insert(
            clusters[merge_i].face_ids.end(),
            clusters[merge_j].face_ids.begin(),
            clusters[merge_j].face_ids.end()
        );
        
        // Recompute centroid for merged cluster
        std::vector<FaceEmbedding> embeddings;
        for (int64_t fid : clusters[merge_i].face_ids) {
            // Find the face embedding from our list
            for (const auto& face : faces) {
                if (face.id == fid) {
                    embeddings.push_back(face.embedding);
                    break;
                }
            }
        }
        clusters[merge_i].centroid = m_impl->compute_centroid(embeddings);
        
        // Remove cluster j
        clusters.erase(clusters.begin() + static_cast<long>(merge_j));
        
        merges_done++;
        if (progress) {
            progress(merges_done, total_faces);
        }
    }
    
    std::cout << "[Clusterer] Created " << clusters.size() << " clusters" << std::endl;
    
    // Save clusters to database
    m_impl->database->begin_transaction();
    
    try {
        for (const auto& wc : clusters) {
            if (static_cast<int>(wc.face_ids.size()) < m_impl->config.min_cluster_size) {
                continue;
            }
            
            // Create cluster record
            Cluster cluster;
            cluster.centroid = wc.centroid;
            cluster.face_count = static_cast<int>(wc.face_ids.size());
            cluster.created_date = get_current_timestamp();
            
            int64_t cluster_id = m_impl->database->insert_cluster(cluster);
            
            // Assign faces to this cluster
            for (int64_t face_id : wc.face_ids) {
                m_impl->database->update_face_cluster(face_id, cluster_id);
            }
        }
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
}

void Clusterer::cluster_new_faces(ProgressCallback progress)
{
    // Get faces without a cluster assignment
    std::vector<Face> unclustered = m_impl->database->get_unclustered_faces();
    
    if (unclustered.empty()) {
        std::cout << "[Clusterer] No unclustered faces" << std::endl;
        return;
    }
    
    std::cout << "[Clusterer] Clustering " << unclustered.size() << " new faces..." << std::endl;
    
    // Get existing clusters
    std::vector<Cluster> existing_clusters = m_impl->database->get_all_clusters();
    
    m_impl->database->begin_transaction();
    
    try {
        int processed = 0;
        int total = static_cast<int>(unclustered.size());
        
        for (const auto& face : unclustered) {
            if (!face.has_embedding()) continue;
            
            // Try to find a matching existing cluster
            auto nearest = m_impl->find_nearest_cluster(face.embedding, existing_clusters);
            
            if (nearest.has_value()) {
                // Add to existing cluster
                m_impl->database->update_face_cluster(face.id, nearest.value());
                m_impl->update_cluster_centroid(nearest.value());
            } else {
                // Create a new cluster for this face
                Cluster cluster;
                cluster.centroid = face.embedding;
                cluster.face_count = 1;
                cluster.created_date = get_current_timestamp();
                
                int64_t cluster_id = m_impl->database->insert_cluster(cluster);
                m_impl->database->update_face_cluster(face.id, cluster_id);
                
                // Add to our working list so subsequent faces can join
                cluster.id = cluster_id;
                existing_clusters.push_back(cluster);
            }
            
            processed++;
            if (progress) {
                progress(processed, total);
            }
        }
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
}

int64_t Clusterer::merge(int64_t cluster_a_id, int64_t cluster_b_id)
{
    if (cluster_a_id == cluster_b_id) {
        return cluster_a_id;  // Same cluster, nothing to do
    }
    
    m_impl->database->begin_transaction();
    
    try {
        // Get faces from cluster B
        std::vector<Face> faces_b = m_impl->database->get_faces_for_cluster(cluster_b_id);
        
        // Move all faces from B to A
        for (const auto& face : faces_b) {
            m_impl->database->update_face_cluster(face.id, cluster_a_id);
        }
        
        // Update centroid of cluster A
        m_impl->update_cluster_centroid(cluster_a_id);
        
        // Delete cluster B
        m_impl->database->delete_cluster(cluster_b_id);
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
    
    return cluster_a_id;
}

int64_t Clusterer::split(int64_t source_cluster_id, const std::vector<int64_t>& face_ids)
{
    if (face_ids.empty()) {
        throw std::invalid_argument("No faces to split");
    }
    
    m_impl->database->begin_transaction();
    
    try {
        // Get embeddings for the faces being moved
        std::vector<FaceEmbedding> embeddings;
        for (int64_t fid : face_ids) {
            auto face = m_impl->database->get_face(fid);
            if (face.has_value() && face->has_embedding()) {
                embeddings.push_back(face->embedding);
            }
        }
        
        // Create new cluster
        Cluster new_cluster;
        new_cluster.centroid = m_impl->compute_centroid(embeddings);
        new_cluster.face_count = static_cast<int>(face_ids.size());
        new_cluster.created_date = get_current_timestamp();
        
        int64_t new_cluster_id = m_impl->database->insert_cluster(new_cluster);
        
        // Move faces to new cluster
        for (int64_t face_id : face_ids) {
            m_impl->database->update_face_cluster(face_id, new_cluster_id);
        }
        
        // Update source cluster centroid
        m_impl->update_cluster_centroid(source_cluster_id);
        
        // Check if source cluster is now empty
        std::vector<Face> remaining = m_impl->database->get_faces_for_cluster(source_cluster_id);
        if (remaining.empty()) {
            m_impl->database->delete_cluster(source_cluster_id);
        }
        
        m_impl->database->commit();
        
        return new_cluster_id;
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
}

void Clusterer::assign_person(int64_t cluster_id, int64_t person_id)
{
    m_impl->database->begin_transaction();
    
    try {
        // Get all faces in this cluster
        std::vector<Face> faces = m_impl->database->get_faces_for_cluster(cluster_id);
        
        // Update each face with the person ID
        for (const auto& face : faces) {
            m_impl->database->update_face_person(face.id, person_id);
        }
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
}

void Clusterer::unassign_person(int64_t cluster_id)
{
    m_impl->database->begin_transaction();
    
    try {
        std::vector<Face> faces = m_impl->database->get_faces_for_cluster(cluster_id);
        
        for (const auto& face : faces) {
            // Set person_id to null by using a special "unset" operation
            // We need to modify Database to support this, for now just note it
            // This would require an update_face_person that accepts optional
        }
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
}

std::optional<Face> Clusterer::get_representative_face(int64_t cluster_id)
{
    auto cluster = m_impl->database->get_cluster(cluster_id);
    if (!cluster.has_value() || cluster->centroid.empty()) {
        return std::nullopt;
    }
    
    std::vector<Face> faces = m_impl->database->get_faces_for_cluster(cluster_id);
    if (faces.empty()) {
        return std::nullopt;
    }
    
    // Find face closest to centroid
    float min_dist = std::numeric_limits<float>::max();
    const Face* best = nullptr;
    
    for (const auto& face : faces) {
        if (!face.has_embedding()) continue;
        
        float dist = FaceService::embedding_distance(face.embedding, cluster->centroid);
        if (dist < min_dist) {
            min_dist = dist;
            best = &face;
        }
    }
    
    if (best) {
        return *best;
    }
    return std::nullopt;
}

std::vector<std::pair<int64_t, int64_t>> Clusterer::get_merge_suggestions(float threshold)
{
    std::vector<std::pair<int64_t, int64_t>> suggestions;
    
    std::vector<Cluster> clusters = m_impl->database->get_all_clusters();
    
    // Find pairs of clusters that are close but not quite at clustering threshold
    for (size_t i = 0; i < clusters.size(); ++i) {
        for (size_t j = i + 1; j < clusters.size(); ++j) {
            if (clusters[i].centroid.empty() || clusters[j].centroid.empty()) {
                continue;
            }
            
            float dist = FaceService::embedding_distance(
                clusters[i].centroid, 
                clusters[j].centroid
            );
            
            // Suggest if distance is between clustering threshold and suggestion threshold
            if (dist > m_impl->config.distance_threshold && dist <= threshold) {
                suggestions.emplace_back(clusters[i].id, clusters[j].id);
            }
        }
    }
    
    return suggestions;
}

std::vector<ClusterStats> Clusterer::get_cluster_stats()
{
    std::vector<ClusterStats> stats;
    
    std::vector<Cluster> clusters = m_impl->database->get_all_clusters();
    
    for (const auto& cluster : clusters) {
        ClusterStats cs;
        cs.cluster_id = cluster.id;
        cs.person_id = cluster.person_id;
        
        if (cluster.person_id.has_value()) {
            auto person = m_impl->database->get_person(cluster.person_id.value());
            if (person.has_value()) {
                cs.person_name = person->name;
            }
        }
        
        std::vector<Face> faces = m_impl->database->get_faces_for_cluster(cluster.id);
        cs.face_count = static_cast<int>(faces.size());
        
        // Count unique photos
        std::set<int64_t> photo_ids;
        for (const auto& face : faces) {
            photo_ids.insert(face.photo_id);
        }
        cs.photo_count = static_cast<int>(photo_ids.size());
        
        // Get representative face
        auto rep = get_representative_face(cluster.id);
        if (rep.has_value()) {
            cs.representative_face_id = rep->id;
        }
        
        stats.push_back(cs);
    }
    
    return stats;
}

void Clusterer::set_threshold(float threshold)
{
    m_impl->config.distance_threshold = threshold;
}

float Clusterer::get_threshold() const
{
    return m_impl->config.distance_threshold;
}

} // namespace facefling
