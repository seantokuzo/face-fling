/**
 * Exporter implementation stub.
 */

#include "Exporter.h"
#include "../services/Database.h"
#include <filesystem>

namespace facefling {

class Exporter::Impl {
public:
    std::shared_ptr<IDatabase> database;
    
    std::string build_filename(
        const std::string& original_path,
        const std::string& person_name,
        const Options& options);
};

Exporter::Exporter(std::shared_ptr<IDatabase> database)
    : m_impl(std::make_unique<Impl>())
{
    m_impl->database = database;
}

Exporter::~Exporter() = default;

void Exporter::export_person(
    int64_t person_id,
    const std::string& destination,
    const Options& options,
    ProgressCallback progress)
{
    // TODO: Implement
    // 1. Query all photos for person
    // 2. For each photo, copy to destination with smart filename
    (void)person_id;
    (void)destination;
    (void)options;
    (void)progress;
}

void Exporter::export_cluster(
    int64_t cluster_id,
    const std::string& destination,
    const Options& options,
    ProgressCallback progress)
{
    // TODO: Implement
    (void)cluster_id;
    (void)destination;
    (void)options;
    (void)progress;
}

std::string Exporter::Impl::build_filename(
    const std::string& original_path,
    const std::string& person_name,
    const Options& options)
{
    namespace fs = std::filesystem;
    
    fs::path path(original_path);
    std::string result;
    
    if (options.include_person_name && !person_name.empty()) {
        result += person_name + options.separator;
    }
    
    if (options.include_original_folder) {
        auto parent = path.parent_path().filename().string();
        if (!parent.empty()) {
            result += parent + options.separator;
        }
    }
    
    result += path.stem().string();
    result += path.extension().string();
    
    return result;
}

} // namespace facefling
