# WarriorPlug-Ins

A collection of four professional audio plugins built with JUCE, designed for musicians and audio engineers who demand high-quality effects with intuitive interfaces.

## Plugins

### 1. WarriorReverb
**Advanced reverb processor with multiple algorithms**

- **5 Reverb Algorithms**: Hall, Room, Plate, Spring, and Shimmer
- **Real-time Parameters**: Room Size, Damping, Diffusion controls
- **Independent Mix Controls**: Separate Wet and Dry level adjustment
- **Professional Quality**: Multi-tap delay network with advanced filtering

### 2. WarriorDelay
**Multi-tap delay with modulation and filtering**

- **4 Independent Delay Taps**: Each with individual time, level, feedback, and panning
- **2 Modulation LFOs**: Multiple waveforms (Sine, Triangle, Square, Saw) for creative effects
- **Tempo Sync Support**: Lock delays to host tempo with note division options
- **Per-tap Filtering**: Individual lowpass filters with cutoff and resonance controls
- **Stereo Field Control**: Pan each delay tap independently

### 3. WarriorDistortion
**Multi-stage distortion with tube modeling**

- **3 Distortion Stages**: Chain multiple distortion algorithms for complex tones
- **5 Distortion Types**: Overdrive, Fuzz, Tube, Bitcrush, and Waveshaper
- **Tone Stack**: 3-band EQ with Bass, Mid, and Treble controls
- **Cabinet Simulation**: Realistic speaker cabinet modeling with adjustable cutoff
- **Asymmetry Control**: Add harmonic complexity and character

### 4. WarriorCompressor
**Transparent compressor with side-chain support**

- **Professional Dynamics**: Threshold, Ratio, Attack, Release, and Knee controls
- **Side-chain Input**: External triggering with high-pass filtering
- **Look-ahead Processing**: Up to 5ms of look-ahead for transparent compression
- **Auto Makeup Gain**: Intelligent level compensation
- **Real-time Metering**: Input, Output, and Gain Reduction meters
- **Mix Control**: Parallel compression capability

## Features

### Shared Technology
- **Professional DSP**: High-quality algorithms optimized for low CPU usage
- **Warrior Look & Feel**: Consistent dark theme with warrior-inspired design
- **Common Utilities**: Shared DSP functions for consistent sound across plugins
- **Parameter Automation**: Full DAW automation support for all parameters
- **Preset Management**: Save and recall your favorite settings

### Build System
- **CMake Integration**: Modern build system with JUCE framework
- **Multi-platform Support**: Windows, macOS, and Linux compatibility
- **Plugin Formats**: VST3, AU, and Standalone applications
- **Professional Packaging**: Ready for distribution

## Requirements

- **JUCE Framework**: Version 7.0.12 or later
- **CMake**: Version 3.15 or higher
- **C++ Compiler**: Supporting C++17 standard

## Building

1. Clone the repository:
```bash
git clone https://github.com/RJF-72/WarriorPlug-Ins.git
cd WarriorPlug-Ins
```

2. Create build directory:
```bash
mkdir build
cd build
```

3. Configure with CMake:
```bash
cmake ..
```

4. Build all plugins:
```bash
cmake --build . --config Release
```

## Installation

After building, the plugins will be available in the `Builds` directory:
- **VST3**: Copy `.vst3` files to your VST3 plugin directory
- **AU**: Copy `.component` files to `/Library/Audio/Plug-Ins/Components/` (macOS)
- **Standalone**: Run the executable directly

## Usage

Each plugin includes:
- **Intuitive Interface**: Professional controls with real-time visual feedback
- **Preset System**: Built-in presets and ability to save your own
- **Parameter Automation**: Full support for DAW automation
- **Low Latency**: Optimized for real-time performance

## License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Support

For support and questions, please open an issue on the GitHub repository.

---

**Warrior Audio - Unleash Your Sound**