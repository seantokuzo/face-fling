#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "Face.h"

namespace facefling {

/**
 * Represents an auto-generated cluster of similar faces.
 */
struct Cluster {
    int64_t id = 0;
    FaceEmbedding centroid;           // Average embedding of all faces
    int face_count = 0;
    std::string created_date;
    std::optional<int64_t> person_id; // Set when user identifies this cluster
    
    // Populated on demand
    std::vector<Face> faces;
    
    bool is_identified() const {
        return person_id.has_value();
    }
};

/**
 * Statistics for a cluster (for UI display).
 */
struct ClusterStats {
    int64_t cluster_id;
    std::optional<int64_t> person_id;
    std::optional<std::string> person_name;
    int face_count = 0;
    int photo_count = 0;              // Unique photos
    int64_t representative_face_id = 0;
};

} // namespace facefling
