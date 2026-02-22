/**
 * ImageLoader implementation using Qt.
 */

#include "ImageLoader.h"
#include <QImage>
#include <QImageReader>
#include <stdexcept>
#include <algorithm>

namespace facefling {

class ImageLoader::Impl {
public:
    std::vector<std::string> extensions = {
        ".jpg", ".jpeg", ".png", ".gif", 
        ".bmp", ".tiff", ".tif", ".heic", ".heif"
    };
};

ImageLoader::ImageLoader()
    : m_impl(std::make_unique<Impl>())
{
}

ImageLoader::~ImageLoader() = default;

Image ImageLoader::load(const std::string& path)
{
    QImage qimg(QString::fromStdString(path));
    
    if (qimg.isNull()) {
        throw std::runtime_error("Failed to load image: " + path);
    }
    
    // Convert to RGB format
    qimg = qimg.convertToFormat(QImage::Format_RGB888);
    
    Image result;
    result.width = qimg.width();
    result.height = qimg.height();
    result.channels = 3;
    
    // Copy pixel data
    const size_t size = static_cast<size_t>(result.width * result.height * result.channels);
    result.data.resize(size);
    
    // QImage stores rows with potential padding, so copy row by row
    const unsigned char* src = qimg.constBits();
    const int bytesPerLine = qimg.bytesPerLine();
    const int rowBytes = result.width * result.channels;
    
    for (int y = 0; y < result.height; ++y) {
        std::copy(
            src + y * bytesPerLine,
            src + y * bytesPerLine + rowBytes,
            result.data.data() + y * rowBytes
        );
    }
    
    return result;
}

void ImageLoader::save_thumbnail(
    const Image& image,
    const BoundingBox& region,
    const std::string& output_path,
    int size)
{
    if (!image.is_valid()) {
        throw std::invalid_argument("Invalid image");
    }
    
    // Create QImage from data
    QImage qimg(
        image.data.data(),
        image.width,
        image.height,
        image.width * image.channels,
        QImage::Format_RGB888
    );
    
    // Crop to region
    QImage cropped = qimg.copy(region.x, region.y, region.width, region.height);
    
    // Scale to thumbnail size
    QImage scaled = cropped.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Save
    if (!scaled.save(QString::fromStdString(output_path))) {
        throw std::runtime_error("Failed to save thumbnail: " + output_path);
    }
}

std::vector<std::string> ImageLoader::supported_extensions() const
{
    return m_impl->extensions;
}

bool ImageLoader::is_supported(const std::string& path) const
{
    // Get extension
    auto pos = path.rfind('.');
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string ext = path.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return std::find(
        m_impl->extensions.begin(), 
        m_impl->extensions.end(), 
        ext
    ) != m_impl->extensions.end();
}

} // namespace facefling
