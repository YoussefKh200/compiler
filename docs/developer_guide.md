# Developer Guide

## Building from Source

### Prerequisites

- CMake 3.16+
- LLVM 15+ (with development headers)
- Python 3.8+ (for AI module)
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)

### Ubuntu/Debian

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake llvm-15-dev libcurl4-openssl-dev nlohmann-json3-dev

# Build
./scripts/build.sh Release

### MacOS7
# Install dependencies
brew install cmake llvm nlohmann-json

# Build
./scripts/build.sh Release


##Running Tests
./scripts/test.sh
