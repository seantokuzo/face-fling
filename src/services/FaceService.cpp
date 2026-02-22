/**
 * FaceService implementation using dlib.
 * TODO: Integrate dlib for actual face detection and embedding.
 */

#include "FaceService.h"
#include <cmath>
#include <stdexcept>

// TODO: Include dlib headers
// #include <dlib/image_processing/frontal_face_detector.h>
// #include <dlib/image_processing.h>
// #include <dlib/dnn.h>

namespace facefling {

class FaceService::Impl {
public:
    Config config;
    bool initialized = false;
    
    // TODO: Add dlib members
    // dlib::frontal_face_detector hog_detector;
    // dlib::shape_predictor shape_pred;
    // anet_type face_encoder;
};

FaceService::FaceService(const Config& config)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->config = config;
}

FaceService::~FaceService() = default;

void FaceService::initialize()
{
    if (m_impl->initialized) return;
    
    // TODO: Load dlib models
    // const auto& dir = m_impl->config.model_dir;
    // dlib::deserialize(dir + "/shape_predictor_68_face_landmarks.dat") >> m_impl->shape_pred;
    // dlib::deserialize(dir + "/dlib_face_recognition_resnet_model_v1.dat") >> m_impl->face_encoder;
    
    m_impl->initialized = true;
}

bool FaceService::is_initialized() const
{
    return m_impl->initialized;
}

std::vector<FaceDetection> FaceService::detect_faces(const Image& image)
{
    if (!image.is_valid()) {
        return {};
    }
    
    if (!m_impl->initialized) {
        initialize();
    }
    
    // TODO: Implement with dlib
    // 1. Convert Image to dlib matrix
    // 2. Run face detector
    // 3. For each face, get landmarks and compute embedding
    // 4. Return results
    
    (void)image;
    return {};
}

std::vector<BoundingBox> FaceService::detect_faces_fast(const Image& image)
{
    if (!image.is_valid()) {
        return {};
    }
    
    // TODO: Implement face detection only (no embedding)
    (void)image;
    return {};
}

std::optional<FaceEmbedding> FaceService::get_embedding(
    const Image& image,
    const BoundingBox& bbox)
{
    if (!image.is_valid()) {
        return std::nullopt;
    }
    
    // TODO: Implement embedding extraction for a specific face
    (void)image;
    (void)bbox;
    return std::nullopt;
}

float FaceService::embedding_distance(const FaceEmbedding& a, const FaceEmbedding& b)
{
    if (a.size() != 128 || b.size() != 128) {
        throw std::invalid_argument("Embeddings must be 128-dimensional");
    }
    
    float sum = 0.0f;
    for (size_t i = 0; i < 128; ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return std::sqrt(sum);
}

bool FaceService::is_same_person(const FaceEmbedding& a, const FaceEmbedding& b, float threshold)
{
    return embedding_distance(a, b) < threshold;
}

} // namespace facefling
