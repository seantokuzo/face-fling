/**
 * Indexer implementation stub.
 * TODO: Implement face detection and embedding pipeline.
 */

#include "Indexer.h"
#include "../services/Database.h"
#include "../services/FaceService.h"
#include "../services/ImageLoader.h"
#include <atomic>

namespace facefling {

class Indexer::Impl {
public:
    std::shared_ptr<IDatabase> database;
    std::shared_ptr<FaceService> face_service;
    std::shared_ptr<ImageLoader> image_loader;
    std::atomic<bool> cancelled{false};
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
    // TODO: Implement
    // For each image:
    // 1. Load image
    // 2. Detect faces
    // 3. Generate embeddings
    // 4. Store in database
    (void)image_paths;
    (void)progress;
}

void Indexer::resume_index(int64_t scan_id, ProgressCallback progress)
{
    // TODO: Implement
    (void)scan_id;
    (void)progress;
}

void Indexer::cancel()
{
    m_impl->cancelled = true;
}

bool Indexer::is_cancelled() const
{
    return m_impl->cancelled;
}

} // namespace facefling
