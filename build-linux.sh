#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Ensure rive-runtime submodule is initialized.
if [ ! -f rive-runtime/build/rive_build_config.lua ]; then
    echo "Initializing rive-runtime submodule..."
    git submodule update --init --recursive
fi

OUTPUT_DIR="build/out/release-linux"
mkdir -p "$OUTPUT_DIR"

echo "Building Linux x64 binary via Docker..."
docker build \
    -f Dockerfile.linux \
    --platform linux/amd64 \
    --target export \
    --output "type=local,dest=$OUTPUT_DIR" \
    .

echo ""
echo "Build complete: $OUTPUT_DIR/rive_code_generator"
file "$OUTPUT_DIR/rive_code_generator" 2>/dev/null || true
