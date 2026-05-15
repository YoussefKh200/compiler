#!/bin/bash
set -e

BUILD_DIR=${1:-build}

cd $BUILD_DIR

echo "Running unit tests..."
ctest --output-on-failure

echo "All tests passed!"
