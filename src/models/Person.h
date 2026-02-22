#pragma once

#include <cstdint>
#include <string>
#include <optional>

namespace facefling {

/**
 * Represents a person identified by the user.
 */
struct Person {
    int64_t id = 0;
    std::string name;
    std::string created_date;
    std::optional<std::string> notes;
    
    // Computed fields (not stored)
    int photo_count = 0;
    int face_count = 0;
    
    bool is_valid() const {
        return id > 0 && !name.empty();
    }
};

} // namespace facefling
