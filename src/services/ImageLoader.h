#pragma once

#include <string>
#include <vector>
#include <memory>
#include "FaceService.h"

namespace facefling {

/**
 * Image loading service.
 * Supports various image formats via Qt.
 */
class ImageLoader {
public:
    ImageLoader();
    ~ImageLoader();
    
    /**
     * Load an image from disk.
     * @param path File path
     * @return Image data (RGB)
     */
    Image load(const std::string& path);
    
    /**
     * Save a thumbnail (cropped region) to disk.
     * @param image Source image
     * @param region Region to crop
     * @param output_path Destination path
     * @param size Output size (square)
     */
    void save_thumbnail(
        const Image& image,
        const BoundingBox& region,
        const std::string& output_path,
        int size = 150
    );
    
    /**
     * Get list of supported file extensions.
     */
    std::vector<std::string> supported_extensions() const;
    
    /**
     * Check if a file is a supported image format.
     */
    bool is_supported(const std::string& path) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
