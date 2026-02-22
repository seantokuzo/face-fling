#!/bin/bash
# setup-models.sh
# Downloads dlib models required for Face-Fling

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODEL_DIR="$SCRIPT_DIR/resources/models"

echo "Setting up Face-Fling models..."
echo "================================"

# Create models directory
mkdir -p "$MODEL_DIR"
cd "$MODEL_DIR"

# Model URLs
MODELS=(
    "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2"
    "http://dlib.net/files/dlib_face_recognition_resnet_model_v1.dat.bz2"
    "http://dlib.net/files/mmod_human_face_detector.dat.bz2"
)

for url in "${MODELS[@]}"; do
    filename=$(basename "$url")
    dat_file="${filename%.bz2}"
    
    if [[ -f "$dat_file" ]]; then
        echo "✓ $dat_file already exists"
    else
        echo "Downloading $filename..."
        curl -O -# "$url"
        
        echo "Extracting..."
        bunzip2 "$filename"
        
        echo "✓ $dat_file downloaded"
    fi
done

echo ""
echo "================================"
echo "Model setup complete!"
echo ""
echo "Files in $MODEL_DIR:"
ls -lh "$MODEL_DIR"
