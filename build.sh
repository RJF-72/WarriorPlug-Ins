#!/bin/bash

# WarriorPlug-Ins Build Script
# Builds all four plugins for release

set -e  # Exit on any error

echo "==============================================="
echo "Building WarriorPlug-Ins Collection"
echo "==============================================="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "Error: CMake is required but not installed."
    exit 1
fi

# Create build directory
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building all plugins..."
cmake --build . --config Release --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo ""
echo "==============================================="
echo "Build completed successfully!"
echo "==============================================="
echo ""
echo "Plugin locations:"
echo "- VST3 plugins: $PWD/*_artefacts/Release/VST3/"
echo "- AU plugins: $PWD/*_artefacts/Release/AU/"
echo "- Standalone apps: $PWD/*_artefacts/Release/Standalone/"
echo ""
echo "Installation:"
echo "- Copy VST3 files to your VST3 plugin directory"
echo "- Copy AU files to /Library/Audio/Plug-Ins/Components/ (macOS)"
echo "- Run standalone applications directly"
echo ""
echo "Happy music making with WarriorPlug-Ins!"