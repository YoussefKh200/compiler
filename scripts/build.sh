#!/bin/bash
set -e

BUILD_TYPE=${1:-Release}
BUILD_DIR="build"

echo "Building compiler in $BUILD_TYPE mode..."

# Create build directory
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Configure
cmake .. -DCMAKE_BUILD_TYPE=$BUILD_TYPE

# Build
cmake --build . --parallel $(nproc)

echo "Build complete. Binary at: $BUILD_DIR/compiler"
