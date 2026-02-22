# SPEC-001: Folder Scanner

> **Status**: `approved`
> **Author**: Face-Fling Team
> **Created**: 2026-02-22
> **Updated**: 2026-02-22

## Summary

The Folder Scanner recursively traverses a user-selected directory to find all image files that can be processed for face detection. It builds a list of image paths while providing progress feedback and respecting system conventions (skipping hidden files, handling permissions).

## Motivation

- Users have photos scattered across nested folder structures (by year, event, etc.)
- Manual traversal is tedious and error-prone
- The scanner provides the foundation for all subsequent face processing
- Progress feedback is essential for large photo libraries (10,000+ images)

## Requirements

### Functional Requirements

| ID    | Requirement                                                                     | Priority |
| ----- | ------------------------------------------------------------------------------- | -------- |
| FR-1  | Accept a root directory path as input                                           | P0       |
| FR-2  | Recursively traverse all subdirectories                                         | P0       |
| FR-3  | Identify image files by extension (.jpg, .jpeg, .png, .heic, .gif, .bmp, .tiff) | P0       |
| FR-4  | Skip hidden files and directories (starting with `.`)                           | P0       |
| FR-5  | Return list of absolute file paths                                              | P0       |
| FR-6  | Provide progress callback with current count, total estimate, and current file  | P0       |
| FR-7  | Handle permission denied gracefully (skip, log, continue)                       | P0       |
| FR-8  | Support cancellation during scan                                                | P1       |
| FR-9  | Detect and handle symbolic link loops                                           | P1       |
| FR-10 | Case-insensitive extension matching                                             | P0       |

### Non-Functional Requirements

| ID    | Requirement                          | Target            |
| ----- | ------------------------------------ | ----------------- |
| NFR-1 | Scan 100,000 files in directory tree | < 30 seconds      |
| NFR-2 | Memory usage during scan             | < 50 MB           |
| NFR-3 | Should not block UI thread           | Background thread |

## Design

### API / Interface

```cpp
// core/Scanner.h

#include <string>
#include <vector>
#include <functional>
#include <atomic>

namespace facefling {

class Scanner {
public:
    // Progress callback: (files_found_so_far, current_directory, current_file)
    using ProgressCallback = std::function<void(
        size_t files_found,
        const std::string& current_dir,
        const std::string& current_file
    )>;

    // Error callback: (path, error_message) - return true to continue, false to abort
    using ErrorCallback = std::function<bool(
        const std::string& path,
        const std::string& error
    )>;

    Scanner();
    ~Scanner();

    // Main scan method
    // Returns vector of absolute paths to image files
    std::vector<std::string> scan(
        const std::string& root_path,
        ProgressCallback progress = nullptr,
        ErrorCallback on_error = nullptr
    );

    // Request cancellation (thread-safe)
    void cancel();

    // Check if cancelled
    bool is_cancelled() const;

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
        std::set<std::string>& visited_dirs  // For symlink loop detection
    );
};

} // namespace facefling
```

### Data Structures

No new data structures required. Returns `std::vector<std::string>`.

### Algorithm

```
FUNCTION scan(root_path, progress_callback, error_callback):
    results = empty list
    visited_dirs = empty set  // For symlink loop detection

    IF not exists(root_path) OR not is_directory(root_path):
        THROW InvalidPathException

    visited_dirs.add(canonical_path(root_path))

    CALL scan_directory(root_path, results, progress_callback, error_callback, visited_dirs)

    RETURN results

FUNCTION scan_directory(dir_path, results, progress, on_error, visited_dirs):
    IF cancelled:
        RETURN

    TRY:
        entries = list_directory(dir_path)
    CATCH permission_denied:
        IF on_error AND NOT on_error(dir_path, "Permission denied"):
            THROW CancelledException
        RETURN

    FOR entry IN entries:
        IF cancelled:
            RETURN

        IF skip_hidden AND is_hidden(entry.name):
            CONTINUE

        IF entry.is_directory:
            IF follow_symlinks AND entry.is_symlink:
                target = canonical_path(entry.path)
                IF target IN visited_dirs:
                    CONTINUE  // Skip symlink loop
                visited_dirs.add(target)

            scan_directory(entry.path, results, progress, on_error, visited_dirs)

        ELSE IF entry.is_regular_file:
            IF is_image_file(entry.path):
                results.add(entry.path)
                IF progress:
                    progress(results.size, dir_path, entry.name)

FUNCTION is_image_file(path):
    extension = lowercase(get_extension(path))
    RETURN extension IN supported_extensions
```

### Dependencies

- `<filesystem>` - C++17 filesystem library
- `<atomic>` - For thread-safe cancellation flag

## UI/UX

### Progress Dialog

```
┌─────────────────────────────────────────────────────┐
│  Scanning for photos...                         [X] │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Found: 1,234 images                               │
│                                                     │
│  Currently scanning:                               │
│  /Users/sean/Photos/2019/Summer Vacation/          │
│                                                     │
│  [█████████████████░░░░░░░░░░░░░░░░░░░░]           │
│                                                     │
│                              [Cancel]              │
└─────────────────────────────────────────────────────┘
```

Note: Progress bar is indeterminate during scan since we don't know total until complete.

### User Flow

1. User clicks "Open Folder..." or uses Cmd+O
2. Native macOS folder picker appears
3. User selects root folder, clicks "Open"
4. Progress dialog appears showing scan progress
5. User can click "Cancel" to abort
6. On completion, dialog closes and main view shows found images

## Test Cases

| ID    | Description                | Input                    | Expected Output          |
| ----- | -------------------------- | ------------------------ | ------------------------ |
| TC-1  | Empty directory            | Empty folder             | Empty list, no errors    |
| TC-2  | Flat directory with images | 10 JPGs in one folder    | List of 10 paths         |
| TC-3  | Nested directories         | 3 levels, images at each | All images found         |
| TC-4  | Mixed file types           | JPGs, PDFs, TXTs         | Only JPGs returned       |
| TC-5  | Hidden files               | `.hidden.jpg`            | Not included             |
| TC-6  | Hidden directories         | `.thumbnails/photo.jpg`  | Not included             |
| TC-7  | Case variations            | `Photo.JPG`, `image.Png` | Both included            |
| TC-8  | Permission denied          | Unreadable folder        | Skip and continue        |
| TC-9  | Symlink to parent          | `link -> ..`             | No infinite loop         |
| TC-10 | Cancellation               | Cancel mid-scan          | Partial results or empty |
| TC-11 | Non-existent path          | `/fake/path`             | Exception thrown         |
| TC-12 | File instead of dir        | Path to file             | Exception thrown         |

## Open Questions

- [x] Should we follow symlinks? → No by default, configurable
- [x] What about HEIC support since macOS uses it? → Yes, included in extensions
- [ ] Should we pre-count files for accurate progress bar? (Adds overhead)

## Implementation Notes

- Use `std::filesystem::recursive_directory_iterator` with `skip_permission_denied`
- Extension matching should be case-insensitive
- On macOS, accessing certain folders (Desktop, Documents) requires permissions
- Consider using `std::filesystem::canonical` for symlink resolution
- Progress callback will be called frequently - UI should throttle updates

## References

- [C++17 Filesystem Library](https://en.cppreference.com/w/cpp/filesystem)
- [macOS File System Overview](https://developer.apple.com/library/archive/documentation/FileManagement/Conceptual/FileSystemProgrammingGuide/FileSystemOverview/FileSystemOverview.html)
