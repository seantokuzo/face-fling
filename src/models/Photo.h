#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace facefling {

/**
 * Represents a photo file in the library.
 */
struct Photo {
    int64_t id = 0;
    std::string file_path;
    std::string file_name;
    std::string folder_path;
    int width = 0;
    int height = 0;
    int64_t file_size = 0;
    std::optional<std::string> exif_date;
    std::string scan_date;
    std::string checksum;
    
    bool is_valid() const {
        return !file_path.empty();
    }
};

} // namespace facefling
