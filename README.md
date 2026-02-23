<p align="center">
  <img src="resources/FaceFling_logo.png" width="500" alt="Face-Fling Logo">
</p>
<p align="center">
  <em>Your family photos, finally findable.</em>
</p>

A native macOS application that scans your photo folders, detects faces, intelligently groups them, and lets you organize photos by the people in them — even when they've aged decades across your collection.

## Features

- **Recursive Scan**: Point at a folder and find all images, even nested deeply
- **Face Detection**: Automatically detect all faces using state-of-the-art ML
- **Smart Clustering**: Group similar faces together automatically
- **Age-Agnostic**: Merge clusters for the same person at different ages
- **Easy Export**: Copy all photos of a person to a new folder with smart naming
- **Privacy First**: Everything runs locally — your photos never leave your machine

## Screenshots

_Coming soon!_

## Requirements

- macOS 10.15 (Catalina) or later
- Intel x86_64 processor (Apple Silicon via Rosetta 2)

## Building from Source

### Prerequisites

```bash
# Install Homebrew if you don't have it
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install Qt 6
brew install qt@6

# Install dlib dependencies
brew install cmake openblas

# Clone and build dlib
git clone https://github.com/davisking/dlib.git
cd dlib
mkdir build && cd build
cmake .. -DUSE_AVX_INSTRUCTIONS=1
cmake --build . --config Release
sudo make install
cd ../..
```

### Download Models

Face-Fling requires dlib model files. Download and extract to `resources/models/`:

```bash
mkdir -p resources/models
cd resources/models

# Face detector
curl -O http://dlib.net/files/mmod_human_face_detector.dat.bz2
bunzip2 mmod_human_face_detector.dat.bz2

# Shape predictor
curl -O http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
bunzip2 shape_predictor_68_face_landmarks.dat.bz2

# Face recognition
curl -O http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2
bunzip2 dlib_face_recognition_resnet_model_v1.dat.bz2

cd ../..
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)
cmake --build .
```

### Run

```bash
./FaceFling.app/Contents/MacOS/FaceFling
```

Or double-click `FaceFling.app` in Finder.

## Usage

1. **Open a folder**: Click "Open Folder..." or press `Cmd+O`
2. **Wait for scan**: The app will find all images and detect faces
3. **Review clusters**: Browse automatically grouped faces
4. **Identify people**: Click a cluster and enter the person's name
5. **Merge clusters**: Select multiple clusters and click "Merge" for same person at different ages
6. **Export photos**: Select a person and click "Export" to copy their photos to a new folder

## Keyboard Shortcuts

| Action         | Shortcut |
| -------------- | -------- |
| Open Folder    | Cmd+O    |
| Export         | Cmd+E    |
| Merge Clusters | Cmd+M    |
| Settings       | Cmd+,    |
| Quit           | Cmd+Q    |

## Technology

- **Language**: C++17
- **GUI**: Qt 6
- **Face Recognition**: dlib (ResNet-based, 99% accuracy on LFW)
- **Database**: SQLite 3
- **Build**: CMake

## Project Structure

```
face-fling/
├── src/               # Source code
│   ├── app/           # Qt UI components
│   ├── core/          # Business logic
│   ├── services/      # External library wrappers
│   └── models/        # Data structures
├── docs/              # Documentation
│   ├── PROJECT_ROADMAP.md
│   ├── ARCHITECTURE.md
│   └── specs/         # Feature specifications
├── resources/         # App resources & models
└── tests/             # Unit tests
```

## Documentation

- [Project Roadmap](docs/PROJECT_ROADMAP.md) - Development phases and requirements
- [Architecture](docs/ARCHITECTURE.md) - Technical design
- [Specs](docs/specs/) - Feature specifications

## Contributing

See [AGENTS.md](AGENTS.md) for development guidelines.

## License

MIT License - see LICENSE file.

---

_Built with ❤️ for finding that one photo of your cousin from 1995._
