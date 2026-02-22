/**
 * Scanner implementation.
 * See docs/specs/001-folder-scanner.md for specification.
 */

#include "Scanner.h"
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace facefling {

Scanner::Scanner()
{
    // Default supported image extensions
    m_extensions = {
        ".jpg", ".jpeg", ".png", ".gif", 
        ".bmp", ".tiff", ".tif", ".heic", ".heif"
    };
}

Scanner::~Scanner() = default;

std::vector<std::string> Scanner::scan(
    const std::string& root_path,
    ProgressCallback progress,
    ErrorCallback on_error)
{
    std::vector<std::string> results;
    results.reserve(10000); // Pre-allocate for performance
    
    // Validate root path
    std::error_code ec;
    if (!fs::exists(root_path, ec) || !fs::is_directory(root_path, ec)) {
        if (on_error) {
            on_error(root_path, "Path does not exist or is not a directory");
        }
        return results;
    }
    
    // Track visited directories for symlink loop detection
    std::set<std::string> visited_dirs;
    
    // Get canonical path of root
    auto canonical = fs::canonical(root_path, ec);
    if (!ec) {
        visited_dirs.insert(canonical.string());
    }
    
    // Start recursive scan
    scan_directory(root_path, results, progress, on_error, visited_dirs);
    
    return results;
}

void Scanner::scan_directory(
    const std::string& dir_path,
    std::vector<std::string>& results,
    ProgressCallback& progress,
    ErrorCallback& on_error,
    std::set<std::string>& visited_dirs)
{
    if (m_cancelled) {
        return;
    }
    
    std::error_code ec;
    
    for (const auto& entry : fs::directory_iterator(dir_path, 
            fs::directory_options::skip_permission_denied, ec)) {
        
        if (m_cancelled) {
            return;
        }
        
        const auto& path = entry.path();
        const auto filename = path.filename().string();
        
        // Skip hidden files/directories
        if (m_skip_hidden && is_hidden(filename)) {
            continue;
        }
        
        if (entry.is_directory(ec)) {
            // Handle symlinks
            if (entry.is_symlink(ec) && m_follow_symlinks) {
                auto target = fs::canonical(path, ec);
                if (!ec) {
                    // Check for symlink loop
                    if (visited_dirs.count(target.string())) {
                        continue;
                    }
                    visited_dirs.insert(target.string());
                }
            }
            
            // Recurse into subdirectory
            scan_directory(path.string(), results, progress, on_error, visited_dirs);
        }
        else if (entry.is_regular_file(ec)) {
            if (is_image_file(path.string())) {
                results.push_back(fs::absolute(path).string());
                
                if (progress) {
                    progress(results.size(), dir_path, filename);
                }
            }
        }
    }
    
    // Handle directory iteration errors
    if (ec && on_error) {
        on_error(dir_path, ec.message());
    }
}

bool Scanner::is_image_file(const std::string& path) const
{
    auto ext = fs::path(path).extension().string();
    
    // Convert to lowercase for comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    
    return std::find(m_extensions.begin(), m_extensions.end(), ext) != m_extensions.end();
}

bool Scanner::is_hidden(const std::string& name) const
{
    return !name.empty() && name[0] == '.';
}

void Scanner::cancel()
{
    m_cancelled = true;
}

bool Scanner::is_cancelled() const
{
    return m_cancelled;
}

void Scanner::reset()
{
    m_cancelled = false;
}

void Scanner::set_extensions(const std::vector<std::string>& extensions)
{
    m_extensions = extensions;
}

std::vector<std::string> Scanner::get_extensions() const
{
    return m_extensions;
}

void Scanner::set_skip_hidden(bool skip)
{
    m_skip_hidden = skip;
}

bool Scanner::get_skip_hidden() const
{
    return m_skip_hidden;
}

void Scanner::set_follow_symlinks(bool follow)
{
    m_follow_symlinks = follow;
}

bool Scanner::get_follow_symlinks() const
{
    return m_follow_symlinks;
}

} // namespace facefling
