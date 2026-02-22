# SPEC-003: Face Clusterer

> **Status**: `approved`
> **Author**: Face-Fling Team
> **Created**: 2026-02-22
> **Updated**: 2026-02-22

## Summary

The Face Clusterer groups detected faces into clusters based on visual similarity. It handles automatic clustering, user-driven merge/split operations, and person identity assignment. Critical for the core use case of finding photos of the same person across different ages.

## Motivation

- Users have photos spanning decades - same person looks different at age 5 vs 35
- Automatic clustering reduces initial manual work
- Merge capability allows user to combine "same person at different ages"
- Split capability fixes incorrect automatic groupings
- Must handle thousands of faces efficiently

## Requirements

### Functional Requirements

| ID    | Requirement                                                | Priority |
| ----- | ---------------------------------------------------------- | -------- |
| FR-1  | Cluster faces based on embedding similarity                | P0       |
| FR-2  | Use configurable distance threshold                        | P0       |
| FR-3  | Compute cluster centroid (average embedding)               | P0       |
| FR-4  | Support manual cluster merge (combine two clusters)        | P0       |
| FR-5  | Support manual cluster split (move faces to new cluster)   | P0       |
| FR-6  | Assign person identity to cluster                          | P0       |
| FR-7  | Find representative face for cluster (closest to centroid) | P1       |
| FR-8  | Provide cluster statistics (face count, photos count)      | P1       |
| FR-9  | Support incremental re-clustering after new scan           | P1       |
| FR-10 | Suggest potential merges based on centroid distance        | P2       |

### Non-Functional Requirements

| ID    | Requirement           | Target       |
| ----- | --------------------- | ------------ |
| NFR-1 | Cluster 10,000 faces  | < 30 seconds |
| NFR-2 | Merge operation       | < 100 ms     |
| NFR-3 | Split operation       | < 100 ms     |
| NFR-4 | Memory for clustering | < 200 MB     |

## Design

### API / Interface

```cpp
// core/Clusterer.h

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include "models/Face.h"
#include "models/Cluster.h"
#include "models/Person.h"

namespace facefling {

// Forward declarations
class IDatabase;
class FaceService;

class Clusterer {
public:
    struct Config {
        float distance_threshold = 0.6f;   // Faces within this distance = same cluster
        int min_cluster_size = 1;          // Minimum faces per cluster
    };

    using ProgressCallback = std::function<void(int processed, int total)>;

    Clusterer(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<FaceService> face_service,
        const Config& config = {}
    );

    // Run clustering on all unclustered faces
    void cluster_all(ProgressCallback progress = nullptr);

    // Cluster new faces only (incremental)
    void cluster_new_faces(ProgressCallback progress = nullptr);

    // Merge two clusters into one
    // Returns the ID of the merged cluster (cluster_a is kept, cluster_b is deleted)
    int64_t merge(int64_t cluster_a_id, int64_t cluster_b_id);

    // Split faces from a cluster into a new cluster
    // Returns the ID of the new cluster containing the moved faces
    int64_t split(int64_t source_cluster_id, const std::vector<int64_t>& face_ids);

    // Assign a person identity to all faces in a cluster
    void assign_person(int64_t cluster_id, int64_t person_id);

    // Remove person assignment from a cluster
    void unassign_person(int64_t cluster_id);

    // Get the face closest to the cluster centroid (best representative)
    std::optional<Face> get_representative_face(int64_t cluster_id);

    // Get clusters that might be the same person (for merge suggestions)
    std::vector<std::pair<int64_t, int64_t>> get_merge_suggestions(
        float threshold = 0.7f  // Slightly higher than clustering threshold
    );

    // Get all clusters with statistics
    struct ClusterStats {
        int64_t cluster_id;
        std::optional<int64_t> person_id;
        std::optional<std::string> person_name;
        int face_count;
        int photo_count;      // Unique photos
        int64_t representative_face_id;
    };
    std::vector<ClusterStats> get_cluster_stats();

    // Configuration
    void set_threshold(float threshold);
    float get_threshold() const;

private:
    std::shared_ptr<IDatabase> m_database;
    std::shared_ptr<FaceService> m_face_service;
    Config m_config;

    // Calculate centroid of a set of embeddings
    std::vector<float> compute_centroid(const std::vector<std::vector<float>>& embeddings);

    // Update cluster's centroid after modification
    void update_cluster_centroid(int64_t cluster_id);

    // Find nearest cluster for a face
    std::optional<int64_t> find_nearest_cluster(
        const std::vector<float>& embedding,
        const std::vector<Cluster>& clusters
    );
};

} // namespace facefling
```

### Algorithm

#### Agglomerative Clustering (Initial)

```
FUNCTION cluster_all(progress_callback):
    // Get all faces with embeddings from database
    faces = database.get_all_faces_with_embeddings()

    IF faces.empty():
        RETURN

    // Start with each face in its own cluster
    clusters = []  // List of (centroid, face_ids)

    FOR face IN faces:
        clusters.append({
            centroid: face.embedding,
            face_ids: [face.id]
        })

    // Iteratively merge closest clusters until no more can be merged
    WHILE True:
        min_distance = INFINITY
        merge_i, merge_j = -1, -1

        // Find closest pair of clusters
        FOR i = 0 TO clusters.size - 1:
            FOR j = i + 1 TO clusters.size - 1:
                dist = euclidean_distance(clusters[i].centroid, clusters[j].centroid)
                IF dist < min_distance:
                    min_distance = dist
                    merge_i, merge_j = i, j

        // Stop if no clusters are close enough
        IF min_distance > threshold:
            BREAK

        // Merge clusters[j] into clusters[i]
        clusters[merge_i].face_ids.extend(clusters[merge_j].face_ids)
        clusters[merge_i].centroid = compute_centroid(
            [get_embedding(f) FOR f IN clusters[merge_i].face_ids]
        )
        clusters.remove(merge_j)

        IF progress_callback:
            progress_callback(original_count - clusters.size, original_count)

    // Save clusters to database
    FOR cluster IN clusters:
        db_cluster = database.create_cluster(cluster.centroid)
        FOR face_id IN cluster.face_ids:
            database.assign_face_to_cluster(face_id, db_cluster.id)
```

#### Optimized Clustering (For Large Datasets)

For 10,000+ faces, use approximate nearest neighbor:

```
FUNCTION cluster_all_optimized(faces):
    // Use DBSCAN-like approach with spatial index

    // Build ball tree or KD-tree for fast nearest neighbor
    tree = build_spatial_index(faces)

    clusters = []
    visited = set()

    FOR face IN faces:
        IF face.id IN visited:
            CONTINUE

        // Find all faces within threshold distance
        neighbors = tree.query_radius(face.embedding, threshold)

        IF neighbors.size < min_cluster_size:
            // Noise point - put in its own cluster
            clusters.append([face.id])
            visited.add(face.id)
        ELSE:
            // Start new cluster and expand
            cluster = expand_cluster(face, neighbors, tree, visited)
            clusters.append(cluster)

    RETURN clusters
```

#### Merge Operation

```
FUNCTION merge(cluster_a_id, cluster_b_id):
    // Validate both clusters exist
    cluster_a = database.get_cluster(cluster_a_id)
    cluster_b = database.get_cluster(cluster_b_id)

    IF NOT cluster_a OR NOT cluster_b:
        THROW ClusterNotFoundException

    // Move all faces from B to A
    faces_b = database.get_faces_for_cluster(cluster_b_id)
    FOR face IN faces_b:
        database.update_face_cluster(face.id, cluster_a_id)

    // Update centroid of A
    all_faces = database.get_faces_for_cluster(cluster_a_id)
    new_centroid = compute_centroid([f.embedding FOR f IN all_faces])
    database.update_cluster_centroid(cluster_a_id, new_centroid)

    // If B had a person assignment, optionally transfer it
    IF cluster_b.person_id AND NOT cluster_a.person_id:
        database.assign_person_to_cluster(cluster_a_id, cluster_b.person_id)

    // Delete empty cluster B
    database.delete_cluster(cluster_b_id)

    RETURN cluster_a_id
```

#### Split Operation

```
FUNCTION split(source_cluster_id, face_ids_to_move):
    // Validate source cluster
    source = database.get_cluster(source_cluster_id)
    IF NOT source:
        THROW ClusterNotFoundException

    // Validate face IDs belong to source cluster
    FOR face_id IN face_ids_to_move:
        face = database.get_face(face_id)
        IF face.cluster_id != source_cluster_id:
            THROW InvalidFaceException

    // Create new cluster
    moved_faces = [database.get_face(id) FOR id IN face_ids_to_move]
    new_centroid = compute_centroid([f.embedding FOR f IN moved_faces])
    new_cluster = database.create_cluster(new_centroid)

    // Move faces to new cluster
    FOR face_id IN face_ids_to_move:
        database.update_face_cluster(face_id, new_cluster.id)

    // Update source cluster centroid
    remaining = database.get_faces_for_cluster(source_cluster_id)
    IF remaining.empty():
        database.delete_cluster(source_cluster_id)
    ELSE:
        remaining_centroid = compute_centroid([f.embedding FOR f IN remaining])
        database.update_cluster_centroid(source_cluster_id, remaining_centroid)

    RETURN new_cluster.id
```

### Centroid Calculation

```cpp
std::vector<float> compute_centroid(const std::vector<std::vector<float>>& embeddings) {
    if (embeddings.empty()) return {};

    const size_t dims = 128;
    std::vector<float> centroid(dims, 0.0f);

    // Sum all embeddings
    for (const auto& emb : embeddings) {
        for (size_t i = 0; i < dims; ++i) {
            centroid[i] += emb[i];
        }
    }

    // Average
    float n = static_cast<float>(embeddings.size());
    for (size_t i = 0; i < dims; ++i) {
        centroid[i] /= n;
    }

    return centroid;
}
```

## Test Cases

| ID    | Description          | Input                 | Expected Output               |
| ----- | -------------------- | --------------------- | ----------------------------- |
| TC-1  | Two identical faces  | Same photo twice      | 1 cluster with 2 faces        |
| TC-2  | Two different people | 2 distinct people     | 2 clusters                    |
| TC-3  | Empty database       | No faces              | No clusters created           |
| TC-4  | Single face          | 1 face                | 1 cluster with 1 face         |
| TC-5  | Merge clusters       | 2 cluster IDs         | 1 cluster with combined faces |
| TC-6  | Merge non-existent   | Invalid ID            | Exception                     |
| TC-7  | Split cluster        | Cluster + face IDs    | New cluster with moved faces  |
| TC-8  | Split all faces      | Move all faces        | New cluster, old deleted      |
| TC-9  | Large dataset        | 10,000 faces          | Completes in < 30s            |
| TC-10 | Threshold too low    | threshold=0.1         | Many small clusters           |
| TC-11 | Threshold too high   | threshold=1.0         | One large cluster             |
| TC-12 | Assign person        | cluster_id, person_id | All faces have person_id      |

## UI Integration

### Cluster View

```
┌─────────────────────────────────────────────────────────────────────┐
│  Clusters                                                      📊   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐            │
│  │ 👤    │  │ 👤    │  │ 👤    │  │ 👤    │  │ 👤    │            │
│  │       │  │       │  │       │  │       │  │       │            │
│  │ 23    │  │ 18    │  │ 12    │  │ 8     │  │ 5     │            │
│  │photos │  │photos │  │photos │  │photos │  │photos │            │
│  └───────┘  └───────┘  └───────┘  └───────┘  └───────┘            │
│  "Unknown" "Unknown" "Unknown"  "John"     "Unknown"              │
│                                                                     │
│  [Select multiple clusters to merge]                               │
│                                                                     │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━ │
│  Selected Cluster Details:                                         │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐           │
│  │ face │ │ face │ │ face │ │ face │ │ face │ │ face │           │
│  │  ☑  │ │  ☑  │ │  ☐  │ │  ☐  │ │  ☐  │ │  ☐  │           │
│  └──────┘ └──────┘ └──────┘ └──────┘ └──────┘ └──────┘           │
│                                                                     │
│  [Name: ________] [Split Selected] [Delete from Cluster]          │
└─────────────────────────────────────────────────────────────────────┘
```

### User Actions

1. **View Clusters**: See all auto-generated clusters with face counts
2. **Name Cluster**: Click cluster, enter person name
3. **Merge**: Select 2+ clusters, click "Merge" (e.g., child + adult same person)
4. **Split**: Select faces within cluster, click "Split" to create new cluster
5. **Reassign**: Drag face from one cluster to another

## Open Questions

- [x] Which clustering algorithm? → Agglomerative (simple, deterministic)
- [ ] Should we suggest "similar clusters" for potential merge?
- [ ] Store clustering history for undo?

## Implementation Notes

- For datasets > 5000 faces, consider approximate nearest neighbor (FAISS, Annoy)
- Centroid should be recomputed whenever cluster membership changes
- UI should batch database updates for better performance
- Consider caching cluster centroids in memory during session

## References

- [DBSCAN Algorithm](https://en.wikipedia.org/wiki/DBSCAN)
- [Agglomerative Clustering](https://scikit-learn.org/stable/modules/clustering.html#hierarchical-clustering)
- [Face Clustering Survey](https://arxiv.org/abs/1906.09066)
