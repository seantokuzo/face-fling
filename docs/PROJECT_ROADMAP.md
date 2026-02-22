# Face-Fling Project Roadmap

> **Your family photos, finally findable.**

A native macOS application that scans folders of photos, detects faces, clusters similar faces together, and lets you organize photos by the people in them.

---

## 🎯 Project Vision

Finding that one picture of your cousin from 1995 shouldn't require manually browsing through thousands of photos. Face-Fling does the heavy lifting: it scans your photo folders, finds every face, intelligently groups similar-looking faces together, and lets you easily identify and organize photos by person—even when they've aged 30 years across your photo collection.

---

## 📋 Core Requirements

### Must Have (MVP)

| ID  | Feature               | Description                                           | Priority |
| --- | --------------------- | ----------------------------------------------------- | -------- |
| R1  | Folder Selection      | User can select a root folder to scan                 | P0       |
| R2  | Recursive Scan        | Find all image files in all subdirectories            | P0       |
| R3  | Face Detection        | Detect all faces in each image                        | P0       |
| R4  | Face Embedding        | Generate vector embeddings for each detected face     | P0       |
| R5  | Face Clustering       | Automatically group similar faces together            | P0       |
| R6  | Person Identification | User can name clusters (assign identity)              | P0       |
| R7  | Cluster Merge         | User can merge clusters (same person, different ages) | P0       |
| R8  | Cluster Split         | User can split incorrectly grouped faces              | P0       |
| R9  | Export by Person      | Copy photos of a selected person to a new folder      | P0       |
| R10 | Progress Indication   | Show scanning/processing progress                     | P0       |

### Should Have (v1.0)

| ID  | Feature         | Description                                         | Priority |
| --- | --------------- | --------------------------------------------------- | -------- |
| R11 | Smart Filenames | Exported files named with person + original context | P1       |
| R12 | Scan Resume     | Resume interrupted scans without starting over      | P1       |
| R13 | Face Thumbnails | Show cropped face thumbnails in UI                  | P1       |
| R14 | Search          | Search for a person by name                         | P1       |
| R15 | Statistics      | Show counts of photos per person                    | P1       |

### Nice to Have (Future)

| ID  | Feature             | Description                               | Priority |
| --- | ------------------- | ----------------------------------------- | -------- |
| R16 | Date Extraction     | Extract dates from EXIF for timeline view | P2       |
| R17 | Multiple Libraries  | Support multiple photo libraries          | P2       |
| R18 | Face Verification   | "Is this really X?" confirmation mode     | P2       |
| R19 | Duplicate Detection | Flag duplicate photos                     | P2       |
| R20 | Photo Preview       | Full image preview with face highlights   | P2       |

---

## 🛠 Technology Stack

### Core Technologies

| Component            | Technology          | Rationale                                                         |
| -------------------- | ------------------- | ----------------------------------------------------------------- |
| **Language**         | C++17               | Performance, direct hardware access, dlib compatibility           |
| **GUI Framework**    | Qt 6                | Cross-platform, native macOS feel, excellent C++ integration      |
| **Face Detection**   | dlib                | Battle-tested, high accuracy, works offline, Intel x86_64 support |
| **Face Recognition** | dlib (ResNet model) | 128-dim embeddings, ~99% accuracy on LFW benchmark                |
| **Database**         | SQLite 3            | Embedded, zero config, perfect for local app data                 |
| **Image Processing** | Qt + stb_image      | Read various image formats (JPEG, PNG, HEIC, etc.)                |
| **Build System**     | CMake               | Industry standard for C++ projects                                |

### Dependencies

```
Qt 6.x          - GUI framework
dlib 19.x       - Face detection/recognition
SQLite 3.x      - Embedded database
```

### Supported Platforms

- **Primary**: macOS (Intel x86_64) — your target
- **Secondary**: macOS (Apple Silicon via Rosetta 2)
- **Future**: Linux, Windows (Qt makes this easy)

---

## 🏗 Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         Face-Fling App                          │
├─────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                     Qt GUI Layer                         │   │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  │   │
│  │  │ Folder  │  │  Face    │  │  Person  │  │  Export  │  │   │
│  │  │ Browser │  │  Grid    │  │  List    │  │  Dialog  │  │   │
│  │  └─────────┘  └──────────┘  └──────────┘  └──────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Core Engine Layer                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │   Scanner   │  │   Indexer   │  │   Clusterer     │  │   │
│  │  │  (folders)  │  │   (faces)   │  │  (grouping)     │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Services Layer                         │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │    dlib     │  │   SQLite    │  │    Image I/O    │  │   │
│  │  │  (ML model) │  │ (database)  │  │   (Qt/stb)      │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

See [ARCHITECTURE.md](./ARCHITECTURE.md) for detailed component design.

---

## 📅 Development Phases

### Phase 0: Project Setup (Week 1)

- [x] Define requirements and roadmap ← **YOU ARE HERE**
- [ ] Set up development environment
- [ ] Configure CMake build system
- [ ] Set up Qt project structure
- [ ] Download and configure dlib models
- [ ] Create SQLite schema

### Phase 1: Core Engine (Weeks 2-3)

- [ ] Implement file scanner (recursive directory walk)
- [ ] Implement image loader (multi-format support)
- [ ] Integrate dlib face detection
- [ ] Integrate dlib face embedding generation
- [ ] Implement SQLite persistence layer
- [ ] Implement face clustering algorithm

### Phase 2: Basic UI (Week 4)

- [ ] Main window layout
- [ ] Folder selection dialog
- [ ] Progress bar during scan
- [ ] Face grid view (unclustered)
- [ ] Person list sidebar

### Phase 3: Clustering UI (Week 5)

- [ ] Clustered face groups display
- [ ] Name assignment dialog
- [ ] Merge clusters (drag & drop or select + merge)
- [ ] Split cluster (select faces to move)
- [ ] Cluster thumbnail generation

### Phase 4: Export & Polish (Week 6)

- [ ] Export dialog (select person, choose destination)
- [ ] Smart filename generation
- [ ] Progress indication for export
- [ ] Settings/preferences
- [ ] Keyboard shortcuts (following macOS HIG)
- [ ] Menu bar implementation

### Phase 5: Testing & Release (Week 7+)

- [ ] Unit tests for core engine
- [ ] Integration tests
- [ ] Performance testing with large libraries
- [ ] Bug fixes and polish
- [ ] Create DMG installer
- [ ] Documentation

---

## 📁 Project Structure

```
face-fling/
├── docs/                      # Documentation
│   ├── PROJECT_ROADMAP.md     # This file
│   ├── ARCHITECTURE.md        # Technical architecture
│   └── specs/                 # Spec-driven development specs
│       ├── SPEC_TEMPLATE.md   # Template for new specs
│       ├── 001-folder-scanner.md
│       ├── 002-face-detector.md
│       └── ...
│
├── src/                       # Source code
│   ├── main.cpp               # Entry point
│   ├── app/                   # Application layer
│   │   ├── MainWindow.cpp
│   │   ├── MainWindow.h
│   │   └── ...
│   ├── core/                  # Core engine
│   │   ├── Scanner.cpp
│   │   ├── Indexer.cpp
│   │   ├── Clusterer.cpp
│   │   └── ...
│   ├── services/              # Service layer
│   │   ├── FaceService.cpp    # dlib wrapper
│   │   ├── Database.cpp       # SQLite wrapper
│   │   └── ImageLoader.cpp
│   └── models/                # Data models
│       ├── Face.h
│       ├── Person.h
│       └── Photo.h
│
├── resources/                 # App resources
│   ├── icons/
│   ├── models/                # dlib model files
│   └── face-fling.qrc         # Qt resource file
│
├── tests/                     # Test code
│   ├── test_scanner.cpp
│   ├── test_clustering.cpp
│   └── ...
│
├── .agents/                   # Copilot configuration
│   ├── skills/                # Installed skills
│   └── workflows/             # Custom workflows
│
├── AGENTS.md                  # Copilot instructions
├── CMakeLists.txt             # Build configuration
├── README.md                  # Project overview
└── .gitignore
```

---

## 🔢 Database Schema

```sql
-- Photos table
CREATE TABLE photos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT UNIQUE NOT NULL,
    file_name TEXT NOT NULL,
    folder_path TEXT NOT NULL,
    width INTEGER,
    height INTEGER,
    file_size INTEGER,
    exif_date TEXT,
    scan_date TEXT NOT NULL,
    checksum TEXT
);

-- Faces table
CREATE TABLE faces (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    photo_id INTEGER NOT NULL,
    bbox_x INTEGER NOT NULL,
    bbox_y INTEGER NOT NULL,
    bbox_width INTEGER NOT NULL,
    bbox_height INTEGER NOT NULL,
    embedding BLOB NOT NULL,  -- 128-dim float vector
    cluster_id INTEGER,
    person_id INTEGER,
    confidence REAL,
    FOREIGN KEY (photo_id) REFERENCES photos(id),
    FOREIGN KEY (cluster_id) REFERENCES clusters(id),
    FOREIGN KEY (person_id) REFERENCES persons(id)
);

-- Clusters table (auto-generated groups)
CREATE TABLE clusters (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    centroid BLOB,            -- Average embedding
    face_count INTEGER DEFAULT 0,
    created_date TEXT NOT NULL
);

-- Persons table (user-defined identities)
CREATE TABLE persons (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    created_date TEXT NOT NULL,
    notes TEXT
);

-- Scan sessions
CREATE TABLE scans (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    root_path TEXT NOT NULL,
    start_date TEXT NOT NULL,
    end_date TEXT,
    status TEXT NOT NULL,     -- 'running', 'completed', 'paused', 'failed'
    total_files INTEGER,
    processed_files INTEGER,
    total_faces INTEGER
);
```

---

## ⚙️ Configuration

### Face Clustering Parameters

| Parameter           | Default                               | Description                                            |
| ------------------- | ------------------------------------- | ------------------------------------------------------ |
| `cluster_threshold` | 0.6                                   | Distance threshold for same-cluster (lower = stricter) |
| `min_face_size`     | 80                                    | Minimum face size in pixels to detect                  |
| `embedding_model`   | dlib_face_recognition_resnet_model_v1 | The face embedding model                               |
| `detection_model`   | mmod_human_face_detector              | CNN face detector                                      |

### Performance Settings

| Parameter        | Default | Description                        |
| ---------------- | ------- | ---------------------------------- |
| `batch_size`     | 10      | Images to process before DB commit |
| `max_threads`    | 4       | Parallel processing threads        |
| `thumbnail_size` | 150     | Face thumbnail size in pixels      |

---

## 🎨 UI/UX Guidelines

This project follows the [macOS Human Interface Guidelines](../.agents/skills/macos-design-guidelines/SKILL.md).

Key points:

- **Menu Bar**: Full menu bar with all actions accessible via keyboard
- **Keyboard Shortcuts**: Cmd+O (open folder), Cmd+E (export), etc.
- **Native Feel**: Use Qt's macOS style, native dialogs
- **Resizable Windows**: All windows freely resizable with sensible minimums
- **Progress Feedback**: Never leave user wondering if app is working
- **Undo/Redo**: Support for merge/split operations

---

## 🚦 Success Criteria

### MVP Complete When:

1. ✅ User can select a folder and scan starts
2. ✅ All images in folder tree are scanned
3. ✅ Faces are detected and displayed in UI
4. ✅ Similar faces are automatically clustered
5. ✅ User can name a cluster (assign person)
6. ✅ User can merge two clusters
7. ✅ User can split a cluster
8. ✅ User can export all photos of a person to a folder

### Performance Targets:

- Scan 10,000 photos in < 1 hour on Intel Mac
- UI remains responsive during scanning
- Database queries return in < 100ms
- App launches in < 2 seconds

---

## 📚 References

- [dlib Face Recognition](http://dlib.net/face_recognition.py.html)
- [Qt 6 Documentation](https://doc.qt.io/qt-6/)
- [SQLite Documentation](https://sqlite.org/docs.html)
- [Apple Human Interface Guidelines](https://developer.apple.com/design/human-interface-guidelines/macos)
- [Face Recognition embedding model](http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2)

---

## 🔄 Status

| Phase                    | Status         | Notes                   |
| ------------------------ | -------------- | ----------------------- |
| Phase 0: Setup           | 🟡 In Progress | Creating roadmap & docs |
| Phase 1: Core            | ⚪ Not Started |                         |
| Phase 2: Basic UI        | ⚪ Not Started |                         |
| Phase 3: Clustering UI   | ⚪ Not Started |                         |
| Phase 4: Export & Polish | ⚪ Not Started |                         |
| Phase 5: Testing         | ⚪ Not Started |                         |

---

_Last Updated: February 22, 2026_
