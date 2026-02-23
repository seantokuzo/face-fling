# Face-Fling Architecture

This document describes the technical architecture of Face-Fling.

## System Overview

```
┌────────────────────────────────────────────────────────────────────────────┐
│                              Face-Fling                                    │
│                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         PRESENTATION LAYER                           │  │
│  │                           (Qt 6 Widgets)                             │  │
│  │                                                                      │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────────┐ │  │
│  │  │MainWindow  │ │ FaceGrid   │ │PersonList  │ │ Dialogs            │ │  │
│  │  │            │ │ Widget     │ │ Widget     │ │ (Export, Settings) │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                    │                                       │
│                                    ▼                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         CORE ENGINE LAYER                            │  │
│  │                       (Business Logic in C++)                        │  │
│  │                                                                      │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────────┐ │  │
│  │  │ Scanner    │ │ Indexer    │ │ Clusterer  │ │ Exporter           │ │  │
│  │  │            │ │            │ │            │ │                    │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                    │                                       │
│                                    ▼                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         SERVICES LAYER                               │  │
│  │                    (External Library Wrappers)                       │  │
│  │                                                                      │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────────────┐ │  │
│  │  │FaceService │ │ Database   │ │ImageLoader │ │ FileSystem         │ │  │
│  │  │  (dlib)    │ │ (SQLite)   │ │ (Qt/stb)   │ │ (std::filesystem)  │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                    │                                       │
│                                    ▼                                       │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                         DATA LAYER                                   │  │
│  │                                                                      │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────────────────────────────┐ │  │
│  │  │ SQLite DB  │ │ Model Files│ │ User's Photo Files (read-only)    │ │  │
│  │  │ (app data) │ │ (dlib)     │ │                                    │ │  │
│  │  └────────────┘ └────────────┘ └────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## Layer Responsibilities

### Presentation Layer (Qt Widgets)

**Purpose**: User interface and user interaction handling.

**Components**:

| Component             | Responsibility                                                |
| --------------------- | ------------------------------------------------------------- |
| `MainWindow`          | Main application window, menu bar, toolbar, layout management |
| `FaceGridWidget`      | Display grid of face thumbnails organized by cluster          |
| `FaceThumbnailWidget` | Individual face thumbnail with selection and circular crop    |
| `PersonListWidget`    | Sidebar showing persons and unidentified clusters             |
| `ScanProgressDialog`  | Show progress during folder scanning and processing           |
| `ExportDialog`        | Configure and execute photo export                            |
| `SettingsDialog`      | Application preferences                                       |
| `ClusterMergeDialog`  | UI for merging face clusters                                  |

**Rules**:

- No business logic in UI code
- UI calls Core layer methods via signals/slots
- All long operations run in background threads
- UI updates via queued signals for thread safety

### Core Engine Layer

**Purpose**: Business logic and orchestration.

**Components**:

| Component        | Responsibility                                  |
| ---------------- | ----------------------------------------------- |
| `Scanner`        | Recursive directory traversal, find image files |
| `Indexer`        | Orchestrate face detection/embedding pipeline   |
| `Clusterer`      | Group similar faces, manage merge/split         |
| `Exporter`       | Copy photos to destination with naming          |
| `Person Manager` | CRUD operations for person identities           |

**Rules**:

- Pure C++, no Qt dependencies (optional: Qt signals for progress)
- Stateless where possible
- Use dependency injection for services
- Report progress via callbacks

### Services Layer

**Purpose**: Wrap external libraries with clean interfaces.

**Components**:

| Component     | External Library | Interface                           |
| ------------- | ---------------- | ----------------------------------- |
| `FaceService` | dlib             | `detect_faces()`, `get_embedding()` |
| `Database`    | SQLite           | `insert()`, `query()`, `update()`   |
| `ImageLoader` | Qt / stb_image   | `load_image()`, `save_thumbnail()`  |
| `FileSystem`  | std::filesystem  | `list_directory()`, `copy_file()`   |

**Rules**:

- Isolate external library details
- Return domain objects, not library types
- Handle library errors, convert to domain exceptions
- Lazy-load expensive resources (dlib models)

### Data Layer

**Purpose**: Persistent storage.

**Components**:

- SQLite database file (`face-fling.db`)
- dlib model files (shipped with app or downloaded)
- User's photo files (read-only access)

---

## Data Flow

### Scan Flow

```
User clicks "Scan Folder"
         │
         ▼
┌─────────────────┐
│  MainWindow     │ ── selectFolder() ──▶ QFileDialog
└─────────────────┘
         │
         │ startScan(path)
         ▼
┌─────────────────┐
│    Scanner      │ ── list_directory() ──▶ FileSystem
└─────────────────┘
         │
         │ for each image file
         ▼
┌─────────────────┐
│    Indexer      │
└─────────────────┘
         │
         ├── load_image() ──▶ ImageLoader
         │
         ├── detect_faces() ──▶ FaceService (dlib)
         │
         ├── get_embedding() ──▶ FaceService (dlib)
         │
         └── insert() ──▶ Database (SQLite)
         │
         │ after all images
         ▼
┌─────────────────┐
│   Clusterer     │ ── cluster_faces() ──▶ Face embeddings
└─────────────────┘
         │
         │ create clusters
         ▼
┌─────────────────┐
│   Database      │ ── update clusters
└─────────────────┘
         │
         │ signal: scanComplete
         ▼
┌─────────────────┐
│  FaceGridWidget │ ── refresh display
└─────────────────┘
```

### Export Flow

```
User selects person, clicks "Export"
         │
         ▼
┌─────────────────┐
│  ExportDialog   │ ── selectDestination() ──▶ QFileDialog
└─────────────────┘
         │
         │ startExport(personId, destPath)
         ▼
┌─────────────────┐
│    Exporter     │ ── query() ──▶ Database (get photos for person)
└─────────────────┘
         │
         │ for each photo
         │
         ├── build_filename() (person name + context)
         │
         └── copy_file() ──▶ FileSystem
         │
         │ signal: exportComplete
         ▼
┌─────────────────┐
│  MainWindow     │ ── show success message
└─────────────────┘
```

---

## Class Design

### Core Models

```cpp
// models/Photo.h
struct Photo {
    int64_t id;
    std::string file_path;
    std::string file_name;
    std::string folder_path;
    int width;
    int height;
    int64_t file_size;
    std::optional<std::string> exif_date;
    std::string scan_date;
    std::string checksum;
};

// models/Face.h
struct BoundingBox {
    int x, y, width, height;
};

struct Face {
    int64_t id;
    int64_t photo_id;
    BoundingBox bbox;
    std::vector<float> embedding;  // 128 dimensions
    std::optional<int64_t> cluster_id;
    std::optional<int64_t> person_id;
    float confidence;
};

// models/Cluster.h
struct Cluster {
    int64_t id;
    std::vector<float> centroid;  // Average embedding
    int face_count;
    std::string created_date;
    std::vector<Face> faces;      // Populated on demand
};

// models/Person.h
struct Person {
    int64_t id;
    std::string name;
    std::string created_date;
    std::optional<std::string> notes;
    int photo_count;              // Computed
};
```

### Service Interfaces

```cpp
// services/IFaceService.h
class IFaceService {
public:
    virtual ~IFaceService() = default;

    // Detect all faces in an image
    virtual std::vector<FaceDetection> detect_faces(const Image& image) = 0;

    // Get 128-dim embedding for a face
    virtual std::vector<float> get_embedding(
        const Image& image,
        const BoundingBox& face_bbox
    ) = 0;

    // Compute distance between two embeddings (0 = identical)
    virtual float embedding_distance(
        const std::vector<float>& a,
        const std::vector<float>& b
    ) = 0;
};

// services/IDatabase.h
class IDatabase {
public:
    virtual ~IDatabase() = default;

    // Photos
    virtual int64_t insert_photo(const Photo& photo) = 0;
    virtual std::optional<Photo> get_photo(int64_t id) = 0;
    virtual std::optional<Photo> get_photo_by_path(const std::string& path) = 0;

    // Faces
    virtual int64_t insert_face(const Face& face) = 0;
    virtual std::vector<Face> get_faces_for_photo(int64_t photo_id) = 0;
    virtual std::vector<Face> get_faces_for_cluster(int64_t cluster_id) = 0;
    virtual std::vector<Face> get_faces_for_person(int64_t person_id) = 0;
    virtual std::vector<Face> get_all_faces_with_embeddings() = 0;
    virtual void update_face_cluster(int64_t face_id, int64_t cluster_id) = 0;
    virtual void update_face_person(int64_t face_id, int64_t person_id) = 0;

    // Clusters
    virtual int64_t insert_cluster(const Cluster& cluster) = 0;
    virtual std::vector<Cluster> get_all_clusters() = 0;
    virtual void delete_cluster(int64_t cluster_id) = 0;
    virtual void merge_clusters(int64_t keep_id, int64_t merge_id) = 0;

    // Persons
    virtual int64_t insert_person(const Person& person) = 0;
    virtual std::vector<Person> get_all_persons() = 0;
    virtual void update_person(const Person& person) = 0;
    virtual void assign_cluster_to_person(int64_t cluster_id, int64_t person_id) = 0;

    // Transactions
    virtual void begin_transaction() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

// services/IImageLoader.h
class IImageLoader {
public:
    virtual ~IImageLoader() = default;

    virtual Image load(const std::string& path) = 0;
    virtual void save_thumbnail(const Image& image, const BoundingBox& region,
                                const std::string& output_path, int size) = 0;
    virtual std::vector<std::string> supported_extensions() = 0;
};
```

### Core Classes

```cpp
// core/Scanner.h
class Scanner {
public:
    using ProgressCallback = std::function<void(int current, int total, const std::string& file)>;

    Scanner(std::shared_ptr<IImageLoader> image_loader);

    // Find all image files recursively
    std::vector<std::string> scan(const std::string& root_path,
                                  ProgressCallback progress = nullptr);

private:
    std::shared_ptr<IImageLoader> m_image_loader;
    std::vector<std::string> m_extensions;
};

// core/Indexer.h
class Indexer {
public:
    using ProgressCallback = std::function<void(int current, int total, const std::string& file, int faces_found)>;

    Indexer(std::shared_ptr<IFaceService> face_service,
            std::shared_ptr<IDatabase> database,
            std::shared_ptr<IImageLoader> image_loader);

    // Process images and extract faces
    void index(const std::vector<std::string>& image_paths,
               ProgressCallback progress = nullptr);

    // Resume from last checkpoint
    void resume_index(int64_t scan_id, ProgressCallback progress = nullptr);

private:
    std::shared_ptr<IFaceService> m_face_service;
    std::shared_ptr<IDatabase> m_database;
    std::shared_ptr<IImageLoader> m_image_loader;
};

// core/Clusterer.h
class Clusterer {
public:
    Clusterer(std::shared_ptr<IFaceService> face_service,
              std::shared_ptr<IDatabase> database);

    // Run clustering on all faces
    void cluster_all(float threshold = 0.6f);

    // Merge two clusters
    void merge(int64_t cluster_a, int64_t cluster_b);

    // Split faces from a cluster into a new cluster
    int64_t split(int64_t source_cluster, const std::vector<int64_t>& face_ids);

    // Assign cluster to person identity
    void identify(int64_t cluster_id, int64_t person_id);

private:
    std::shared_ptr<IFaceService> m_face_service;
    std::shared_ptr<IDatabase> m_database;

    std::vector<float> compute_centroid(const std::vector<Face>& faces);
};

// core/Exporter.h
class Exporter {
public:
    struct ExportOptions {
        bool include_person_name = true;
        bool include_original_folder = true;
        bool include_date = true;
        std::string separator = "_";
    };

    using ProgressCallback = std::function<void(int current, int total)>;

    Exporter(std::shared_ptr<IDatabase> database);

    // Export all photos containing a person
    void export_person(int64_t person_id,
                       const std::string& destination,
                       const ExportOptions& options = {},
                       ProgressCallback progress = nullptr);

private:
    std::shared_ptr<IDatabase> m_database;

    std::string build_filename(const Photo& photo,
                               const Person& person,
                               const ExportOptions& options);
};
```

---

## Threading Model

```
┌────────────────────────────────────────────────────────────────┐
│                         Main Thread                            │
│                       (Qt Event Loop)                          │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    UI Updates Only                        │  │
│  │  - Handle user input                                      │  │
│  │  - Update widgets                                         │  │
│  │  - Show dialogs                                          │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
                              │
                              │ Signals (queued)
                              ▼
┌────────────────────────────────────────────────────────────────┐
│                       Worker Threads                           │
│                      (QThreadPool)                             │
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                   Heavy Operations                        │  │
│  │  - Folder scanning                                        │  │
│  │  - Face detection (dlib)                                  │  │
│  │  - Face embedding generation                              │  │
│  │  - Clustering computation                                 │  │
│  │  - File copying                                          │  │
│  └──────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
```

**Thread Safety Rules**:

1. Database access is serialized (SQLite not thread-safe by default)
2. dlib operations can run in parallel (separate model instances)
3. UI updates only from main thread (use `QMetaObject::invokeMethod`)
4. Progress callbacks are thread-safe (use queued signals)

---

## Error Handling Strategy

```cpp
// Common exception hierarchy
namespace facefling {

class Exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DatabaseException : public Exception {
public:
    using Exception::Exception;
};

class ImageLoadException : public Exception {
public:
    ImageLoadException(const std::string& path, const std::string& reason)
        : Exception("Failed to load image: " + path + " - " + reason)
        , m_path(path) {}

    const std::string& path() const { return m_path; }
private:
    std::string m_path;
};

class FaceDetectionException : public Exception {
public:
    using Exception::Exception;
};

} // namespace facefling
```

**Handling Strategy**:

- Services throw specific exceptions
- Core layer catches and logs, may re-throw or continue
- UI layer catches all, shows user-friendly message
- Never silently ignore errors

---

## Configuration

```cpp
// Config stored in SQLite config table or JSON file
struct Config {
    // Clustering
    float cluster_threshold = 0.6f;    // Lower = stricter matching
    int min_face_size = 80;            // Pixels

    // Performance
    int batch_size = 10;               // Images per DB commit
    int max_threads = 4;               // Parallel workers
    int thumbnail_size = 150;          // Pixels

    // Paths
    std::string model_path;            // dlib models directory
    std::string database_path;         // SQLite database file

    // UI
    bool show_confidence_scores = false;
    int grid_thumbnail_size = 120;
};
```

---

## Performance Considerations

### Memory Management

- Load images one at a time for embedding extraction
- Cache face thumbnails on disk, not in memory
- Limit face grid to visible items (virtual scrolling)
- Clear dlib model from memory when not actively scanning

### Database Optimization

- Create indexes on frequently queried columns
- Use transactions for batch inserts
- Vacuum database periodically
- Use prepared statements

```sql
CREATE INDEX idx_faces_photo ON faces(photo_id);
CREATE INDEX idx_faces_cluster ON faces(cluster_id);
CREATE INDEX idx_faces_person ON faces(person_id);
CREATE INDEX idx_photos_path ON photos(file_path);
```

### Batch Processing

- Process images in batches of 10 before committing to DB
- Group face detection calls (dlib can process multiple at once)
- Pre-compute thumbnails lazily (on first view)

---

## Security & Privacy

1. **No network access**: App works entirely offline
2. **No data exfiltration**: Photos never leave user's machine
3. **Read-only source access**: Never modify original photos
4. **Safe filenames**: Sanitize when exporting
5. **Permission respect**: Handle access denied gracefully

---

## Future Extensibility

The architecture supports future enhancements:

- **Multiple face models**: Swap `FaceService` implementation
- **Cloud sync**: Add cloud database backend
- **Other platforms**: Core engine is platform-agnostic
- **Plugins**: Add plugin interface for custom exporters
