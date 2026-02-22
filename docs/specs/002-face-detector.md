# SPEC-002: Face Detector

> **Status**: `approved`
> **Author**: Face-Fling Team
> **Created**: 2026-02-22
> **Updated**: 2026-02-22

## Summary

The Face Detector uses dlib's CNN-based face detection to find all faces in an image and generate 128-dimensional embeddings for each face. These embeddings enable face comparison and clustering.

## Motivation

- Core functionality: must detect faces before we can organize by person
- dlib offers state-of-the-art detection with offline capability
- 128-dim embeddings enable efficient similarity comparison
- Need to handle varying image sizes, quality, and face orientations

## Requirements

### Functional Requirements

| ID    | Requirement                                             | Priority |
| ----- | ------------------------------------------------------- | -------- |
| FR-1  | Detect all faces in an image                            | P0       |
| FR-2  | Return bounding box (x, y, width, height) for each face | P0       |
| FR-3  | Generate 128-dimensional embedding for each face        | P0       |
| FR-4  | Return confidence score for each detection              | P0       |
| FR-5  | Support common image formats (JPEG, PNG, etc.)          | P0       |
| FR-6  | Handle images of varying sizes                          | P0       |
| FR-7  | Filter detections below minimum size threshold          | P0       |
| FR-8  | Filter detections below confidence threshold            | P0       |
| FR-9  | Calculate Euclidean distance between embeddings         | P0       |
| FR-10 | Lazy-load models on first use                           | P1       |

### Non-Functional Requirements

| ID    | Requirement                         | Target                  |
| ----- | ----------------------------------- | ----------------------- |
| NFR-1 | Face detection time per 1080p image | < 2 seconds (CPU)       |
| NFR-2 | Embedding generation per face       | < 500 ms                |
| NFR-3 | Model loading time                  | < 5 seconds             |
| NFR-4 | Memory usage with models loaded     | < 500 MB                |
| NFR-5 | False positive rate                 | < 1%                    |
| NFR-6 | Detection recall (faces found)      | > 95% for frontal faces |

## Design

### API / Interface

```cpp
// services/FaceService.h

#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace facefling {

// Bounding box for detected face
struct BoundingBox {
    int x;          // Left edge
    int y;          // Top edge
    int width;      // Width in pixels
    int height;     // Height in pixels

    // Convenience methods
    int right() const { return x + width; }
    int bottom() const { return y + height; }
    int area() const { return width * height; }
    int center_x() const { return x + width / 2; }
    int center_y() const { return y + height / 2; }
};

// Result of face detection
struct FaceDetection {
    BoundingBox bbox;                   // Face location
    float confidence;                   // Detection confidence [0, 1]
    std::vector<float> embedding;       // 128-dim face embedding
    std::vector<std::pair<int, int>> landmarks;  // 68 facial landmarks (optional)
};

// Image data (simple wrapper)
struct Image {
    std::vector<unsigned char> data;    // RGB pixel data
    int width;
    int height;
    int channels;                       // Usually 3 (RGB)

    bool is_valid() const {
        return !data.empty() && width > 0 && height > 0;
    }
};

// Face detection and embedding service
class FaceService {
public:
    struct Config {
        std::string model_dir;          // Path to dlib model files
        int min_face_size = 80;         // Minimum face size in pixels
        float min_confidence = 0.5f;    // Minimum detection confidence
        int upsample_count = 1;         // Upsampling for small faces (0, 1, or 2)
    };

    FaceService(const Config& config);
    ~FaceService();

    // Initialize models (can be called explicitly or lazily on first detect)
    void initialize();
    bool is_initialized() const;

    // Main detection method
    // Returns all faces found in the image with embeddings
    std::vector<FaceDetection> detect_faces(const Image& image);

    // Detect faces only (no embeddings - faster for preview)
    std::vector<BoundingBox> detect_faces_fast(const Image& image);

    // Get embedding for a specific face region
    // Use when you have the bbox but not the embedding
    std::optional<std::vector<float>> get_embedding(
        const Image& image,
        const BoundingBox& face_bbox
    );

    // Calculate distance between two embeddings
    // Returns Euclidean distance (0 = identical, typically < 0.6 = same person)
    static float embedding_distance(
        const std::vector<float>& embedding_a,
        const std::vector<float>& embedding_b
    );

    // Check if two faces are likely the same person
    static bool is_same_person(
        const std::vector<float>& embedding_a,
        const std::vector<float>& embedding_b,
        float threshold = 0.6f
    );

private:
    class Impl;  // PIMPL to hide dlib headers
    std::unique_ptr<Impl> m_impl;
    Config m_config;
};

} // namespace facefling
```

### Data Structures

```cpp
// Embedding type alias for clarity
using FaceEmbedding = std::vector<float>;  // Always 128 elements

// For batch processing
struct FaceDetectionResult {
    std::string image_path;
    int image_width;
    int image_height;
    std::vector<FaceDetection> faces;
    std::string error;  // Non-empty if processing failed
};
```

### Algorithm

#### Face Detection Pipeline

```
FUNCTION detect_faces(image):
    IF NOT initialized:
        initialize()

    // Convert to dlib image format
    dlib_image = convert_to_dlib_matrix(image)

    // Run CNN face detector
    // This finds face bounding boxes
    detections = cnn_face_detector(dlib_image, upsample_count)

    results = []

    FOR detection IN detections:
        // Filter by confidence
        IF detection.confidence < min_confidence:
            CONTINUE

        // Filter by size
        IF detection.rect.width < min_face_size:
            CONTINUE

        // Get 68 facial landmarks for alignment
        shape = shape_predictor(dlib_image, detection.rect)

        // Compute face embedding using aligned face
        face_chip = extract_aligned_face(dlib_image, shape, size=150)
        embedding = face_encoder(face_chip)  // 128-dim vector

        result = FaceDetection{
            bbox: convert_rect(detection.rect),
            confidence: detection.confidence,
            embedding: vector_from_dlib(embedding),
            landmarks: extract_landmarks(shape)
        }

        results.append(result)

    RETURN results
```

#### Embedding Distance

```
FUNCTION embedding_distance(a, b):
    // Euclidean distance
    sum = 0
    FOR i = 0 TO 127:
        diff = a[i] - b[i]
        sum += diff * diff
    RETURN sqrt(sum)

FUNCTION is_same_person(a, b, threshold):
    RETURN embedding_distance(a, b) < threshold
```

### dlib Models Required

| Model File                                  | Size    | Purpose                   |
| ------------------------------------------- | ------- | ------------------------- |
| `mmod_human_face_detector.dat`              | ~2 MB   | CNN face detector         |
| `shape_predictor_68_face_landmarks.dat`     | ~100 MB | Facial landmark detection |
| `dlib_face_recognition_resnet_model_v1.dat` | ~23 MB  | Face embedding network    |

**Download URLs**:

- http://dlib.net/files/mmod_human_face_detector.dat.bz2
- http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
- http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2

### Dependencies

- **dlib 19.x** - Face detection and recognition
- **Qt** (optional) - For `QImage` loading support

## Implementation Details

### PIMPL Implementation

```cpp
// services/FaceService.cpp

#include "FaceService.h"
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/dnn.h>
#include <dlib/image_io.h>

// dlib DNN network definitions (from dlib examples)
template <template <int,template<typename>class,int,typename> class block, int N, template<typename>class BN, typename SUBNET>
using residual = dlib::add_prev1<block<N,BN,1,dlib::tag1<SUBNET>>>;

// ... (full ResNet definition - see dlib face_recognition example)

using anet_type = /* ResNet definition */;

namespace facefling {

class FaceService::Impl {
public:
    dlib::cnn_face_detection_model_v1 cnn_detector;
    dlib::shape_predictor shape_pred;
    anet_type face_encoder;
    bool initialized = false;

    void load_models(const std::string& model_dir) {
        dlib::deserialize(model_dir + "/mmod_human_face_detector.dat") >> cnn_detector;
        dlib::deserialize(model_dir + "/shape_predictor_68_face_landmarks.dat") >> shape_pred;
        dlib::deserialize(model_dir + "/dlib_face_recognition_resnet_model_v1.dat") >> face_encoder;
        initialized = true;
    }
};

} // namespace facefling
```

### Thread Safety

- `FaceService` instances are NOT thread-safe
- For parallel processing, create one `FaceService` per thread
- Models can be shared read-only after initialization (advanced)

## Test Cases

| ID    | Description       | Input                     | Expected Output            |
| ----- | ----------------- | ------------------------- | -------------------------- |
| TC-1  | Single face       | Photo with 1 person       | 1 detection with embedding |
| TC-2  | Multiple faces    | Group photo (5 people)    | 5 detections               |
| TC-3  | No faces          | Landscape photo           | Empty list                 |
| TC-4  | Small face        | Face < 80px               | Filtered out               |
| TC-5  | Large image       | 4K photo                  | All faces detected         |
| TC-6  | Profile face      | Side view                 | May or may not detect      |
| TC-7  | Same person twice | Two photos of same person | Distances < 0.6            |
| TC-8  | Different people  | Two different people      | Distances > 0.6            |
| TC-9  | Low quality       | Blurry photo              | Lower confidence scores    |
| TC-10 | Corrupted image   | Invalid image data        | Graceful error             |
| TC-11 | Model not found   | Bad model path            | Exception on init          |

## Performance Notes

### CPU Performance (Intel)

On Intel Mac (without GPU acceleration):

- CNN face detector: ~1-2 seconds per 1080p image
- Shape prediction: ~10ms per face
- Face embedding: ~100ms per face

### Optimization Tips

1. **Batch faces for embedding**: dlib can encode multiple face chips at once
2. **Downscale large images**: 1080p is sufficient for most faces
3. **Skip embedding for preview**: Use `detect_faces_fast()` for UI preview
4. **Cache loaded models**: Don't reload between images

### Memory Usage

- Detector model: ~100 MB
- Shape predictor: ~100 MB
- Face encoder: ~100 MB
- Per-image working memory: ~50 MB for 4K image

## Open Questions

- [x] Should we support GPU acceleration? → Not for v1 (Intel target)
- [x] Use CNN or HOG detector? → CNN (more accurate, we accept speed tradeoff)
- [ ] Should we offer quality presets (fast/accurate)?

## References

- [dlib Face Recognition Tutorial](http://dlib.net/face_recognition.py.html)
- [dlib CNN Face Detector](http://blog.dlib.net/2016/10/easily-create-high-quality-object.html)
- [Face Recognition Accuracy](https://arxiv.org/abs/1512.03385)
