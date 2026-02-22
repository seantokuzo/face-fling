# Face-Fling Development Instructions

This file contains instructions for AI coding assistants working on the Face-Fling project.

## Project Overview

Face-Fling is a native macOS application for organizing family photos by the people in them. It uses face detection and recognition to automatically cluster similar faces, allowing users to identify people and export their photos.

## Technology Stack

- **Language**: C++17
- **GUI**: Qt 6
- **Face Recognition**: dlib
- **Database**: SQLite 3
- **Build System**: CMake
- **Target Platform**: macOS (Intel x86_64 primary)

## Development Approach

### Spec-Driven Development

This project follows **Spec-Driven Development (SDD)**. Before implementing any feature:

1. Check for an existing spec in `docs/specs/`
2. If no spec exists, create one using `docs/specs/SPEC_TEMPLATE.md`
3. Get spec approval before coding
4. Reference the spec ID in code comments

### Code Style

- **C++17** standard features encouraged
- Use `snake_case` for functions and variables
- Use `PascalCase` for classes and types
- Use `SCREAMING_SNAKE_CASE` for constants and macros
- Prefer `std::string` over C strings
- Prefer smart pointers over raw pointers
- Use RAII for resource management

### File Organization

```
src/
├── main.cpp           # Entry point only
├── app/               # Qt UI code (MainWindow, dialogs, widgets)
├── core/              # Business logic (Scanner, Indexer, Clusterer)
├── services/          # External integrations (dlib, SQLite)
└── models/            # Data structures (Face, Person, Photo)
```

### Qt Guidelines

- Follow Qt naming conventions for signals/slots
- Use Qt's parent-child ownership for widgets
- Prefer Qt containers when interfacing with Qt, STL otherwise
- Use `QThread` for background processing, keep UI responsive

### macOS Guidelines

Follow the macOS Human Interface Guidelines. Key requirements:

1. **Menu Bar**: Implement full menu bar with standard menus (App, File, Edit, View, Window, Help)
2. **Keyboard Shortcuts**: Every action must have a keyboard shortcut
3. **Native Dialogs**: Use `QFileDialog::getExistingDirectory()` etc.
4. **Window Management**: Support resize, fullscreen, multiple windows where appropriate
5. **Progress Feedback**: Always show progress for long operations

See `.agents/skills/macos-design-guidelines/SKILL.md` for detailed guidelines.

## Key Algorithms

### Face Clustering

The clustering algorithm uses dlib's 128-dimensional face embeddings:

1. For each face, compute embedding using ResNet model
2. Use agglomerative clustering with Euclidean distance
3. Threshold: 0.6 (configurable) - faces within this distance are same cluster
4. Compute cluster centroid as mean of all embeddings

### File Scanning

Recursive directory traversal:

1. Use `std::filesystem` for cross-platform compatibility
2. Filter by image extensions: `.jpg`, `.jpeg`, `.png`, `.heic`, `.gif`, `.bmp`, `.tiff`
3. Skip hidden files and directories (starting with `.`)
4. Track progress via callback

## Database Schema

The SQLite database stores:

- **photos**: Image file metadata
- **faces**: Detected faces with bounding boxes and embeddings
- **clusters**: Auto-generated face groups
- **persons**: User-assigned identities
- **scans**: Scan session tracking for resume capability

See `docs/PROJECT_ROADMAP.md` for full schema.

## Testing

- Unit tests in `tests/` directory
- Use Google Test or Qt Test framework
- Test clustering algorithm with known face datasets
- Performance test with 10,000+ images

## Common Tasks

### Adding a New Feature

1. Create spec in `docs/specs/NNN-feature-name.md`
2. Design the API/interface
3. Implement in appropriate layer (core vs service vs app)
4. Add unit tests
5. Update documentation

### Debugging Face Detection

- dlib's `mmod_human_face_detector` works best with images 800px+
- For small faces, consider upscaling image 2x
- Check `confidence` score - reject below 0.5

### Performance Optimization

- Use batch processing for face detection (multiple images per dlib call)
- Use threading for parallel image loading
- Pre-compute thumbnails lazily
- Index SQLite columns used in WHERE clauses

## Dependencies Setup

### macOS (Homebrew)

```bash
# Install Qt 6
brew install qt@6

# Install dlib dependencies
brew install cmake openblas

# Build dlib from source (for best performance)
git clone https://github.com/davisking/dlib.git
cd dlib
mkdir build && cd build
cmake .. -DUSE_AVX_INSTRUCTIONS=1
cmake --build . --config Release
sudo make install
```

### dlib Models

Download and place in `resources/models/`:

- `shape_predictor_68_face_landmarks.dat` - Face landmark detection
- `dlib_face_recognition_resnet_model_v1.dat` - Face embedding model
- `mmod_human_face_detector.dat` - CNN face detector

## Error Handling

- Use exceptions for exceptional conditions
- Return `std::optional` for expected failures
- Log errors with context (file path, operation, etc.)
- Never silently fail - inform user of problems

## Security Considerations

- Never store photos in database, only paths
- Sanitize file paths to prevent directory traversal
- Handle malformed images gracefully (corrupted files)
- Respect file permissions

## Resources

- Project Roadmap: `docs/PROJECT_ROADMAP.md`
- Architecture: `docs/ARCHITECTURE.md`
- Feature Specs: `docs/specs/`
- macOS HIG: `.agents/skills/macos-design-guidelines/SKILL.md`
