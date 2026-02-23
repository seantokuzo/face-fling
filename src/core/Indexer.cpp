/**
 * Indexer implementation.
 * Orchestrates face detection and embedding generation pipeline.
 */

#include "Indexer.h"
#include "../services/Database.h"
#include "../services/FaceService.h"
#include "../services/ImageLoader.h"
#include <atomic>
#include <filesystem>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace facefling {

// Get current timestamp as ISO string
static std::string get_current_timestamp() {
    auto now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&now), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

class Indexer::Impl {
public:
    std::shared_ptr<IDatabase> database;
    std::shared_ptr<FaceService> face_service;
    std::shared_ptr<ImageLoader> image_loader;
    std::atomic<bool> cancelled{false};
    std::string thumbnail_dir;
    int thumbnail_size = 150;
    
    // Generate thumbnail path for a face
    std::string get_thumbnail_path(int64_t face_id) const {
        return thumbnail_dir + "/face_" + std::to_string(face_id) + ".jpg";
    }
    
    // Process a single image and store results in database
    int process_image(const std::string& image_path, const Image& image) {
        try {
            // Check if photo already exists in database
            auto existing = database->get_photo_by_path(image_path);
            if (existing.has_value()) {
                // Already indexed, skip
                return 0;
            }
            
            // Create photo record
            Photo photo;
            photo.file_path = image_path;
            
            fs::path p(image_path);
            photo.file_name = p.filename().string();
            photo.folder_path = p.parent_path().string();
            photo.width = image.width;
            photo.height = image.height;
            
            // Get file size
            std::error_code ec;
            photo.file_size = static_cast<int64_t>(fs::file_size(p, ec));
            if (ec) {
                photo.file_size = 0;
            }
            
            photo.scan_date = get_current_timestamp();
            
            // Insert photo and get ID
            int64_t photo_id = database->insert_photo(photo);
            
            // Detect faces
            std::vector<FaceDetection> detections = face_service->detect_faces(image);
            
            // Store each detected face
            for (const auto& detection : detections) {
                Face face;
                face.photo_id = photo_id;
                face.bbox = detection.bbox;
                face.embedding = detection.embedding;
                face.confidence = detection.confidence;
                // cluster_id and person_id remain unset (will be assigned during clustering)
                
                int64_t face_id = database->insert_face(face);
                
                // Generate thumbnail
                if (!thumbnail_dir.empty()) {
                    try {
                        // Expand bounding box slightly for better crop
                        BoundingBox expanded_bbox = detection.bbox;
                        int expand = static_cast<int>(expanded_bbox.width * 0.2);
                        expanded_bbox.x = std::max(0, expanded_bbox.x - expand);
                        expanded_bbox.y = std::max(0, expanded_bbox.y - expand);
                        expanded_bbox.width = std::min(image.width - expanded_bbox.x, 
                                                       expanded_bbox.width + expand * 2);
                        expanded_bbox.height = std::min(image.height - expanded_bbox.y, 
                                                        expanded_bbox.height + expand * 2);
                        
                        std::string thumb_path = get_thumbnail_path(face_id);
                        image_loader->save_thumbnail(image, expanded_bbox, thumb_path, thumbnail_size);
                    } catch (const std::exception& e) {
                        std::cerr << "[Indexer] Failed to save thumbnail for face " 
                                  << face_id << ": " << e.what() << std::endl;
                    }
                }
            }
            
            return static_cast<int>(detections.size());
            
        } catch (const std::exception& e) {
            std::cerr << "[Indexer] Error processing " << image_path << ": " << e.what() << std::endl;
            return 0;
        }
    }
};

Indexer::Indexer(
    std::shared_ptr<IDatabase> database,
    std::shared_ptr<FaceService> face_service,
    std::shared_ptr<ImageLoader> image_loader)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->database = database;
    m_impl->face_service = face_service;
    m_impl->image_loader = image_loader;
}

Indexer::~Indexer() = default;

void Indexer::index(
    const std::vector<std::string>& image_paths,
    ProgressCallback progress)
{
    m_impl->cancelled = false;
    
    // Initialize face service if not already done
    if (!m_impl->face_service->is_initialized()) {
        if (progress) {
            progress(0, static_cast<int>(image_paths.size()), "Loading face detection models...", 0);
        }
        m_impl->face_service->initialize();
    }
    
    int total = static_cast<int>(image_paths.size());
    int total_faces = 0;
    
    // Use transactions for better performance with batch inserts
    m_impl->database->begin_transaction();
    
    try {
        for (int i = 0; i < total; ++i) {
            if (m_impl->cancelled) {
                m_impl->database->rollback();
                return;
            }
            
            const std::string& path = image_paths[i];
            
            // Load image first
            Image image;
            try {
                image = m_impl->image_loader->load(path);
            } catch (const std::exception& e) {
                std::cerr << "[Indexer] Failed to load image " << path << ": " << e.what() << std::endl;
                if (progress) {
                    progress(i + 1, total, path, 0);
                }
                continue;
            }
            
            // Process image
            int faces_found = m_impl->process_image(path, image);
            total_faces += faces_found;
            
            // Report progress
            if (progress) {
                progress(i + 1, total, path, total_faces);
            }
            
            // Commit in batches of 50 for better performance
            if ((i + 1) % 50 == 0) {
                m_impl->database->commit();
                m_impl->database->begin_transaction();
            }
        }
        
        m_impl->database->commit();
        
    } catch (...) {
        m_impl->database->rollback();
        throw;
    }
    
    std::cout << "[Indexer] Indexing complete. Processed " << total 
              << " images, found " << total_faces << " faces." << std::endl;
}

void Indexer::resume_index(int64_t scan_id, ProgressCallback progress)
{
    // TODO: Load scan session from database and resume from last checkpoint
    // For now, this is a placeholder
    (void)scan_id;
    (void)progress;
    
    std::cerr << "[Indexer] Resume not yet implemented" << std::endl;
}

void Indexer::cancel()
{
    m_impl->cancelled = true;
}

bool Indexer::is_cancelled() const
{
    return m_impl->cancelled;
}

void Indexer::set_thumbnail_dir(const std::string& dir)
{
    m_impl->thumbnail_dir = dir;
}

void Indexer::set_thumbnail_size(int size)
{
    m_impl->thumbnail_size = size;
}

} // namespace facefling
