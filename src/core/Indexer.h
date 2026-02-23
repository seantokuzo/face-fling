#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "../models/Face.h"

namespace facefling {

// Forward declarations
class IDatabase;
class FaceService;
class ImageLoader;

/**
 * Orchestrates face detection and embedding generation.
 */
class Indexer {
public:
    // Progress callback: (current, total, file, faces_found)
    using ProgressCallback = std::function<void(
        int current, int total, 
        const std::string& file, 
        int faces_found
    )>;
    
    Indexer(
        std::shared_ptr<IDatabase> database,
        std::shared_ptr<FaceService> face_service,
        std::shared_ptr<ImageLoader> image_loader
    );
    ~Indexer();
    
    /**
     * Process images and extract faces.
     * @param image_paths List of image file paths
     * @param progress Optional progress callback
     */
    void index(
        const std::vector<std::string>& image_paths,
        ProgressCallback progress = nullptr
    );
    
    /**
     * Resume from last checkpoint.
     * @param scan_id ID of the scan session to resume
     * @param progress Optional progress callback
     */
    void resume_index(int64_t scan_id, ProgressCallback progress = nullptr);
    
    // Cancel current operation
    void cancel();
    bool is_cancelled() const;
    
    // Thumbnail settings
    void set_thumbnail_dir(const std::string& dir);
    void set_thumbnail_size(int size);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
