#!/bin/bash

# Warrior USB Recorder - Build Script
echo "Building Warrior USB Recorder Plugin..."

# Create build directory
mkdir -p build
cd build

# Check dependencies
echo "Checking dependencies..."

# Check for cmake
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake is required but not installed."
    exit 1
fi

# Check for pkg-config
if ! command -v pkg-config &> /dev/null; then
    echo "Error: pkg-config is required but not installed."
    exit 1
fi

# Check for required libraries
echo "Checking for PortAudio..."
if ! pkg-config --exists portaudio-2.0; then
    echo "Error: PortAudio development libraries not found."
    echo "Install with: sudo apt-get install libportaudio19-dev"
    exit 1
fi

echo "Checking for libusb..."
if ! pkg-config --exists libusb-1.0; then
    echo "Error: libusb development libraries not found."
    echo "Install with: sudo apt-get install libusb-1.0-0-dev"
    exit 1
fi

# Download VST3 SDK if not present
if [ ! -d "../third_party/vst3sdk" ]; then
    echo "Downloading VST3 SDK..."
    mkdir -p ../third_party
    cd ../third_party
    git clone https://github.com/steinbergmedia/vst3sdk.git
    cd vst3sdk
    git submodule update --init --recursive
    cd ../../build
fi

# Configure with CMake
echo "Configuring build..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building plugin..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "Plugin built in: $(pwd)/WarriorUSBRecorder.vst3"
else
    echo "Build failed!"
    exit 1
fi