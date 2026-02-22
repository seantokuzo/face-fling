#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "../models/Face.h"

namespace facefling {

/**
 * Simple image structure for face detection.
 */
struct Image {
    std::vector<unsigned char> data;  // RGB pixel data
    int width = 0;
    int height = 0;
    int channels = 3;  // Usually RGB
    
    bool is_valid() const {
        return !data.empty() && width > 0 && height > 0;
    }
};

/**
 * Face detection and embedding service using dlib.
 * See docs/specs/002-face-detector.md for specification.
 */
class FaceService {
public:
    struct Config {
        std::string model_dir;          // Path to dlib model files
        int min_face_size = 80;         // Minimum face size in pixels
        float min_confidence = 0.5f;    // Minimum detection confidence
        int upsample_count = 1;         // Upsampling for small faces
    };
    
    explicit FaceService(const Config& config);
    ~FaceService();
    
    /**
     * Initialize models (lazy or explicit).
     */
    void initialize();
    bool is_initialized() const;
    
    /**
     * Detect all faces in an image.
     * @return Vector of face detections with embeddings
     */
    std::vector<FaceDetection> detect_faces(const Image& image);
    
    /**
     * Detect faces without embeddings (faster for preview).
     */
    std::vector<BoundingBox> detect_faces_fast(const Image& image);
    
    /**
     * Get embedding for a specific face region.
     */
    std::optional<FaceEmbedding> get_embedding(
        const Image& image,
        const BoundingBox& face_bbox
    );
    
    /**
     * Calculate Euclidean distance between embeddings.
     * 0 = identical, typically < 0.6 = same person
     */
    static float embedding_distance(
        const FaceEmbedding& a,
        const FaceEmbedding& b
    );
    
    /**
     * Check if two faces are likely the same person.
     */
    static bool is_same_person(
        const FaceEmbedding& a,
        const FaceEmbedding& b,
        float threshold = 0.6f
    );

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
