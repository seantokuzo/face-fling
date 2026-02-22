#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <set>

namespace facefling {

/**
 * Recursive folder scanner to find image files.
 * See docs/specs/001-folder-scanner.md for detailed specification.
 */
class Scanner {
public:
    // Progress callback: (files_found, current_dir, current_file)
    using ProgressCallback = std::function<void(
        size_t files_found,
        const std::string& current_dir,
        const std::string& current_file
    )>;
    
    // Error callback: (path, error) - return true to continue, false to abort
    using ErrorCallback = std::function<bool(
        const std::string& path,
        const std::string& error
    )>;
    
    Scanner();
    ~Scanner();
    
    /**
     * Scan a directory recursively for image files.
     * @param root_path Directory to scan
     * @param progress Optional progress callback
     * @param on_error Optional error callback
     * @return Vector of absolute paths to image files
     */
    std::vector<std::string> scan(
        const std::string& root_path,
        ProgressCallback progress = nullptr,
        ErrorCallback on_error = nullptr
    );
    
    // Request cancellation (thread-safe)
    void cancel();
    
    // Check if cancelled
    bool is_cancelled() const;
    
    // Reset cancellation flag for reuse
    void reset();
    
    // Configuration
    void set_extensions(const std::vector<std::string>& extensions);
    std::vector<std::string> get_extensions() const;
    
    void set_skip_hidden(bool skip);
    bool get_skip_hidden() const;
    
    void set_follow_symlinks(bool follow);
    bool get_follow_symlinks() const;

private:
    std::vector<std::string> m_extensions;
    bool m_skip_hidden = true;
    bool m_follow_symlinks = false;
    std::atomic<bool> m_cancelled{false};
    
    bool is_image_file(const std::string& path) const;
    bool is_hidden(const std::string& name) const;
    void scan_directory(
        const std::string& dir_path,
        std::vector<std::string>& results,
        ProgressCallback& progress,
        ErrorCallback& on_error,
        std::set<std::string>& visited_dirs
    );
};

} // namespace facefling
