#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include "../models/Face.h"
#include "../models/Cluster.h"

namespace facefling {

// Forward declarations
class IDatabase;
class FaceService;

/**
 * Groups similar faces into clusters.
 * See docs/specs/003-face-clusterer.md for specification.
 */
class Clusterer {
public:
    struct Config {
        float distance_threshold = 0.6f;  // Faces within this distance = same cluster
        int min_cluster_size = 1;         // Minimum faces per cluster
    };
    
    using ProgressCallback = std::function<void(int processed, int total)>;
    
    Clusterer(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<FaceService> face_service,
        const Config& config = {}
    );
    ~Clusterer();
    
    /**
     * Run clustering on all unclustered faces.
     */
    void cluster_all(ProgressCallback progress = nullptr);
    
    /**
     * Cluster only new (unclustered) faces.
     */
    void cluster_new_faces(ProgressCallback progress = nullptr);
    
    /**
     * Merge two clusters into one.
     * @return ID of the merged cluster
     */
    int64_t merge(int64_t cluster_a_id, int64_t cluster_b_id);
    
    /**
     * Split faces from a cluster into a new cluster.
     * @return ID of the new cluster
     */
    int64_t split(int64_t source_cluster_id, const std::vector<int64_t>& face_ids);
    
    /**
     * Assign a person identity to all faces in a cluster.
     */
    void assign_person(int64_t cluster_id, int64_t person_id);
    
    /**
     * Remove person assignment from a cluster.
     */
    void unassign_person(int64_t cluster_id);
    
    /**
     * Get the face closest to cluster centroid.
     */
    std::optional<Face> get_representative_face(int64_t cluster_id);
    
    /**
     * Get clusters that might be the same person.
     */
    std::vector<std::pair<int64_t, int64_t>> get_merge_suggestions(float threshold = 0.7f);
    
    /**
     * Get statistics for all clusters.
     */
    std::vector<ClusterStats> get_cluster_stats();
    
    // Configuration
    void set_threshold(float threshold);
    float get_threshold() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
