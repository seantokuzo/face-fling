#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace facefling {

/**
 * Bounding box for a detected face.
 */
struct BoundingBox {
    int x = 0;      // Left edge
    int y = 0;      // Top edge
    int width = 0;  // Width in pixels
    int height = 0; // Height in pixels
    
    int right() const { return x + width; }
    int bottom() const { return y + height; }
    int area() const { return width * height; }
    int center_x() const { return x + width / 2; }
    int center_y() const { return y + height / 2; }
};

/**
 * Face embedding - 128-dimensional vector from dlib.
 */
using FaceEmbedding = std::vector<float>;

/**
 * Represents a detected face in a photo.
 */
struct Face {
    int64_t id = 0;
    int64_t photo_id = 0;
    BoundingBox bbox;
    FaceEmbedding embedding;           // 128 dimensions
    std::optional<int64_t> cluster_id;
    std::optional<int64_t> person_id;
    float confidence = 0.0f;
    
    bool has_embedding() const {
        return embedding.size() == 128;
    }
    
    bool is_clustered() const {
        return cluster_id.has_value();
    }
    
    bool is_identified() const {
        return person_id.has_value();
    }
};

/**
 * Result of face detection on a single image.
 */
struct FaceDetection {
    BoundingBox bbox;
    float confidence = 0.0f;
    FaceEmbedding embedding;
    std::vector<std::pair<int, int>> landmarks;  // 68 facial landmarks
};

} // namespace facefling
