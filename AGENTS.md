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

---

## C++ Primer for JavaScript/TypeScript Developers

This section explains C++ concepts for developers coming from JavaScript/TypeScript.

### Key Differences from JS/TS

| Concept         | JavaScript/TypeScript               | C++                                |
| --------------- | ----------------------------------- | ---------------------------------- |
| Memory          | Garbage collected                   | Manual (but we use smart pointers) |
| Types           | Runtime (TS: compile-time optional) | Compile-time, strict               |
| Compilation     | Interpreted/JIT                     | Compiled to machine code           |
| Execution       | Single-threaded + event loop        | Multi-threaded                     |
| Package Manager | npm/yarn                            | CMake + system packages            |

### Header Files (.h) vs Source Files (.cpp)

**Why two files?** C++ separates _declaration_ (what exists) from _definition_ (how it works).

```cpp
// Scanner.h - DECLARATION (the "interface")
class Scanner {
public:
    std::vector<std::string> scan(const std::string& path);  // Just signature
};

// Scanner.cpp - DEFINITION (the "implementation")
std::vector<std::string> Scanner::scan(const std::string& path) {
    // Actual code goes here
}
```

**Think of it like:** `.h` = TypeScript interface, `.cpp` = implementation.

### Memory Management

**JavaScript:** Objects live until garbage collected.
**C++:** You control when objects are created and destroyed.

```cpp
// Stack allocation (automatic cleanup when scope ends)
Photo photo;  // Created here, destroyed at end of function

// Heap allocation with smart pointers (like JS objects, but explicit)
std::shared_ptr<Database> db = std::make_shared<Database>("path.db");
// db is automatically deleted when no references remain (like JS!)

std::unique_ptr<Scanner> scanner = std::make_unique<Scanner>();
// Only ONE owner allowed - like a non-copyable object
```

**Rule:** We always use smart pointers (`shared_ptr`, `unique_ptr`) instead of raw `new`/`delete`.

### Common Types

```cpp
// Strings
std::string name = "John";           // Like: let name: string = "John"
QString qname = "John";              // Qt's string (for UI)

// Arrays/Lists
std::vector<int> numbers = {1, 2, 3};  // Like: let numbers: number[] = [1, 2, 3]
numbers.push_back(4);                   // Like: numbers.push(4)

// Optional values (like T | undefined)
std::optional<Photo> photo = get_photo(123);
if (photo.has_value()) {               // Like: if (photo !== undefined)
    use(photo.value());
}

// Integers
int count = 42;                        // Like: let count: number = 42
int64_t bigNumber = 9999999999LL;      // For large numbers (photo IDs, etc.)
```

### References and Pointers

```cpp
// Reference (&) - an alias to existing object (can't be null)
void process(const std::string& path);  // Read-only reference (efficient, no copy)
void modify(std::string& path);         // Can modify the original

// Pointer (*) - address of object (can be null)
Photo* photo = nullptr;                 // Like: let photo: Photo | null = null
if (photo != nullptr) {
    photo->name;                        // Arrow for pointer access
}
```

**Rule:** Prefer `&` references. Use pointers only when null is valid.

### The `const` Keyword

```cpp
const int MAX = 100;                    // Like: const MAX = 100 (can't change)

void read_only(const std::string& s);   // Promise not to modify s

const Photo& get_photo() const;         // Method doesn't modify the object
```

### Classes and Structs

```cpp
// struct = all public by default (use for simple data)
struct Photo {
    int64_t id;
    std::string path;
};

// class = all private by default (use for objects with behavior)
class Scanner {
public:                                 // Anyone can access
    void scan();

private:                                // Only this class can access
    bool m_cancelled;                   // m_ prefix = member variable
};
```

### Namespaces

```cpp
namespace facefling {                   // Like: module facefling { ... }
    class Scanner { };
}

facefling::Scanner scanner;             // Like: facefling.Scanner
```

### Lambda Functions

```cpp
// C++ lambdas are like JS arrow functions
auto callback = [](int x) { return x * 2; };  // Like: const callback = (x) => x * 2

// Capture variables from outer scope
int multiplier = 3;
auto fn = [multiplier](int x) { return x * multiplier; };
// [&] = capture by reference, [=] = capture by copy, [this] = capture this
```

### Qt Signals and Slots (Event System)

```cpp
// Like addEventListener but type-safe
connect(button, &QPushButton::clicked,    // When button emits "clicked"
        this, &MainWindow::onButtonClick); // Call this method

// Emitting signals (like dispatchEvent)
emit progressChanged(50, 100);            // Notify listeners
```

### Build Process

```
Source Files (.cpp, .h)
        ↓
    [Compiler]  ← CMake tells compiler what to build
        ↓
  Object Files (.o)
        ↓
    [Linker]    ← Combines objects + libraries
        ↓
   Executable (FaceFling.app)
```

**CMake** is like `package.json` + build scripts combined. It:

- Finds dependencies (Qt, dlib, SQLite)
- Configures compiler flags
- Generates platform-specific build files

### Common Gotchas

1. **Semicolons everywhere** - Every statement ends with `;`
2. **No implicit type conversion** - `int` to `string` requires explicit conversion
3. **Include order matters** - Headers must be included before use
4. **Compile errors are verbose** - The first error is usually the real one
5. **Segfault** - Accessing invalid memory (use smart pointers to avoid!)

### Reading This Codebase

```
src/
├── main.cpp              # Entry point - creates MainWindow and runs Qt app
├── app/                  # UI layer (Qt widgets)
│   ├── MainWindow.cpp    # Controller - connects UI to core engine
│   ├── FaceGridWidget.cpp# Displays face thumbnails
│   └── ...
├── core/                 # Business logic (pure C++, no Qt)
│   ├── Scanner.cpp       # Finds image files
│   ├── Indexer.cpp       # Detects faces, generates embeddings
│   └── Clusterer.cpp     # Groups similar faces
├── services/             # External library wrappers
│   ├── Database.cpp      # SQLite operations
│   ├── FaceService.cpp   # dlib face detection
│   └── ImageLoader.cpp   # Image loading
└── models/               # Data structures (structs)
    ├── Photo.h           # Photo metadata
    ├── Face.h            # Detected face with embedding
    └── Person.h          # User-assigned identity
```
