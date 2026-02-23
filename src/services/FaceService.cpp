/**
 * FaceService implementation using dlib.
 * See docs/specs/002-face-detector.md for specification.
 */

#include "FaceService.h"
#include <cmath>
#include <stdexcept>
#include <iostream>

// dlib headers
#include <dlib/dnn.h>
#include <dlib/image_processing.h>
#include <dlib/image_processing/frontal_face_detector.h>

namespace facefling {

// ============================================================================
// dlib ResNet network definition for face recognition
// This is the standard 29-layer ResNet from dlib's face_recognition example
// ============================================================================

template <template <int, template<typename>class, int, typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = dlib::add_prev1<block<N,BN,1,dlib::tag1<SUBNET>>>;

template <template <int, template<typename>class, int, typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual_down = dlib::add_prev2<dlib::avg_pool<2,2,2,2,dlib::skip1<dlib::tag2<block<N,BN,2,dlib::tag1<SUBNET>>>>>>;

template <int N, template <typename> class BN, int stride, typename SUBNET>
using block = BN<dlib::con<N,3,3,1,1,dlib::relu<BN<dlib::con<N,3,3,stride,stride,SUBNET>>>>>;

template <int N, typename SUBNET> using ares = dlib::relu<residual<block,N,dlib::affine,SUBNET>>;
template <int N, typename SUBNET> using ares_down = dlib::relu<residual_down<block,N,dlib::affine,SUBNET>>;

template <typename SUBNET> using alevel0 = ares_down<256,SUBNET>;
template <typename SUBNET> using alevel1 = ares<256,ares<256,ares_down<256,SUBNET>>>;
template <typename SUBNET> using alevel2 = ares<128,ares<128,ares_down<128,SUBNET>>>;
template <typename SUBNET> using alevel3 = ares<64,ares<64,ares<64,ares_down<64,SUBNET>>>>;
template <typename SUBNET> using alevel4 = ares<32,ares<32,ares<32,SUBNET>>>;

using anet_type = dlib::loss_metric<dlib::fc_no_bias<128,dlib::avg_pool_everything<
    alevel0<
    alevel1<
    alevel2<
    alevel3<
    alevel4<
    dlib::max_pool<3,3,2,2,dlib::relu<dlib::affine<dlib::con<32,7,7,2,2,
    dlib::input_rgb_image_sized<150>
    >>>>>>>>>>>>;

// ============================================================================
// FaceService::Impl - private implementation
// ============================================================================

class FaceService::Impl {
public:
    Config config;
    bool initialized = false;
    
    // dlib detectors and networks
    dlib::frontal_face_detector hog_detector;           // Fast HOG detector for fallback
    dlib::shape_predictor shape_predictor;              // 68-point landmark detector
    anet_type face_encoder;                              // ResNet face encoder
    
    // Converts our Image struct to dlib's rgb_image format
    dlib::matrix<dlib::rgb_pixel> to_dlib_image(const Image& image) {
        dlib::matrix<dlib::rgb_pixel> dlib_img(image.height, image.width);
        
        const unsigned char* src = image.data.data();
        for (int y = 0; y < image.height; ++y) {
            for (int x = 0; x < image.width; ++x) {
                const int idx = (y * image.width + x) * image.channels;
                dlib_img(y, x) = dlib::rgb_pixel(src[idx], src[idx + 1], src[idx + 2]);
            }
        }
        return dlib_img;
    }
    
    // Converts dlib rectangle to our BoundingBox
    BoundingBox rect_to_bbox(const dlib::rectangle& rect) {
        BoundingBox bbox;
        bbox.x = static_cast<int>(rect.left());
        bbox.y = static_cast<int>(rect.top());
        bbox.width = static_cast<int>(rect.width());
        bbox.height = static_cast<int>(rect.height());
        return bbox;
    }
    
    // Extracts 68 facial landmarks as coordinate pairs
    std::vector<std::pair<int, int>> extract_landmarks(const dlib::full_object_detection& shape) {
        std::vector<std::pair<int, int>> landmarks;
        landmarks.reserve(shape.num_parts());
        for (unsigned long i = 0; i < shape.num_parts(); ++i) {
            const auto& pt = shape.part(i);
            landmarks.emplace_back(static_cast<int>(pt.x()), static_cast<int>(pt.y()));
        }
        return landmarks;
    }
};

// ============================================================================
// FaceService implementation
// ============================================================================

FaceService::FaceService(const Config& config)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->config = config;
}

FaceService::~FaceService() = default;

void FaceService::initialize()
{
    if (m_impl->initialized) {
        return;
    }
    
    const auto& model_dir = m_impl->config.model_dir;
    
    try {
        std::cout << "[FaceService] Loading HOG face detector..." << std::endl;
        m_impl->hog_detector = dlib::get_frontal_face_detector();
        
        std::cout << "[FaceService] Loading shape predictor from: " 
                  << model_dir + "/shape_predictor_68_face_landmarks.dat" << std::endl;
        dlib::deserialize(model_dir + "/shape_predictor_68_face_landmarks.dat") 
            >> m_impl->shape_predictor;
        
        std::cout << "[FaceService] Loading face encoder from: "
                  << model_dir + "/dlib_face_recognition_resnet_model_v1.dat" << std::endl;
        dlib::deserialize(model_dir + "/dlib_face_recognition_resnet_model_v1.dat") 
            >> m_impl->face_encoder;
        
        m_impl->initialized = true;
        std::cout << "[FaceService] Models loaded successfully." << std::endl;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Failed to load dlib models: ") + e.what() +
            "\nModel directory: " + model_dir +
            "\nMake sure all model files are present."
        );
    }
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
    
    std::vector<FaceDetection> results;
    
    // Convert image to dlib format
    dlib::matrix<dlib::rgb_pixel> dlib_img = m_impl->to_dlib_image(image);
    
    // Detect faces using HOG detector
    // The upsample_count parameter upsamples the image for detecting smaller faces
    std::vector<dlib::rectangle> face_rects = m_impl->hog_detector(
        dlib_img, 
        m_impl->config.upsample_count
    );
    
    // Process each detected face
    for (const auto& rect : face_rects) {
        // Filter by minimum size
        if (rect.width() < m_impl->config.min_face_size || 
            rect.height() < m_impl->config.min_face_size) {
            continue;
        }
        
        // Get facial landmarks
        dlib::full_object_detection shape = m_impl->shape_predictor(dlib_img, rect);
        
        // Extract aligned face chip for embedding
        dlib::matrix<dlib::rgb_pixel> face_chip;
        dlib::extract_image_chip(
            dlib_img, 
            dlib::get_face_chip_details(shape, 150, 0.25), 
            face_chip
        );
        
        // Compute 128-dimensional face embedding
        dlib::matrix<float, 0, 1> face_descriptor = m_impl->face_encoder(face_chip);
        
        // Build result
        FaceDetection detection;
        detection.bbox = m_impl->rect_to_bbox(rect);
        detection.confidence = 1.0f;  // HOG detector doesn't provide confidence, assume high
        detection.landmarks = m_impl->extract_landmarks(shape);
        
        // Convert dlib embedding to std::vector<float>
        detection.embedding.resize(128);
        for (int i = 0; i < 128; ++i) {
            detection.embedding[i] = face_descriptor(i);
        }
        
        results.push_back(std::move(detection));
    }
    
    return results;
}

std::vector<BoundingBox> FaceService::detect_faces_fast(const Image& image)
{
    if (!image.is_valid()) {
        return {};
    }
    
    if (!m_impl->initialized) {
        initialize();
    }
    
    std::vector<BoundingBox> results;
    
    // Convert image to dlib format
    dlib::matrix<dlib::rgb_pixel> dlib_img = m_impl->to_dlib_image(image);
    
    // Detect faces using HOG detector (fast, no embedding)
    std::vector<dlib::rectangle> face_rects = m_impl->hog_detector(
        dlib_img, 
        m_impl->config.upsample_count
    );
    
    // Convert and filter results
    for (const auto& rect : face_rects) {
        if (rect.width() >= m_impl->config.min_face_size && 
            rect.height() >= m_impl->config.min_face_size) {
            results.push_back(m_impl->rect_to_bbox(rect));
        }
    }
    
    return results;
}

std::optional<FaceEmbedding> FaceService::get_embedding(
    const Image& image,
    const BoundingBox& bbox)
{
    if (!image.is_valid()) {
        return std::nullopt;
    }
    
    if (!m_impl->initialized) {
        initialize();
    }
    
    // Convert image to dlib format
    dlib::matrix<dlib::rgb_pixel> dlib_img = m_impl->to_dlib_image(image);
    
    // Create dlib rectangle from bbox
    dlib::rectangle rect(bbox.x, bbox.y, bbox.x + bbox.width - 1, bbox.y + bbox.height - 1);
    
    // Clamp rectangle to image bounds
    rect = rect.intersect(dlib::rectangle(0, 0, image.width - 1, image.height - 1));
    if (rect.is_empty()) {
        return std::nullopt;
    }
    
    // Get facial landmarks
    dlib::full_object_detection shape = m_impl->shape_predictor(dlib_img, rect);
    
    // Extract aligned face chip
    dlib::matrix<dlib::rgb_pixel> face_chip;
    dlib::extract_image_chip(
        dlib_img, 
        dlib::get_face_chip_details(shape, 150, 0.25), 
        face_chip
    );
    
    // Compute embedding
    dlib::matrix<float, 0, 1> face_descriptor = m_impl->face_encoder(face_chip);
    
    // Convert to std::vector
    FaceEmbedding embedding(128);
    for (int i = 0; i < 128; ++i) {
        embedding[i] = face_descriptor(i);
    }
    
    return embedding;
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
