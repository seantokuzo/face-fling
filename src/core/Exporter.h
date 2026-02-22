#pragma once

#include <memory>
#include <string>
#include <functional>
#include <cstdint>

namespace facefling {

// Forward declarations
class IDatabase;

/**
 * Exports photos for a person to a destination folder.
 */
class Exporter {
public:
    struct Options {
        bool include_person_name = true;
        bool include_original_folder = true;
        bool include_date = true;
        std::string separator = "_";
    };
    
    using ProgressCallback = std::function<void(int current, int total)>;
    
    explicit Exporter(std::shared_ptr<IDatabase> database);
    ~Exporter();
    
    /**
     * Export all photos containing a person.
     * @param person_id ID of the person to export
     * @param destination Destination folder path
     * @param options Export options
     * @param progress Optional progress callback
     */
    void export_person(
        int64_t person_id,
        const std::string& destination,
        const Options& options = {},
        ProgressCallback progress = nullptr
    );
    
    /**
     * Export photos from a specific cluster.
     */
    void export_cluster(
        int64_t cluster_id,
        const std::string& destination,
        const Options& options = {},
        ProgressCallback progress = nullptr
    );

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace facefling
