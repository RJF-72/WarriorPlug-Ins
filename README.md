# Warrior USB Recorder Plugin

A comprehensive USB recording plugin for Titan7 DAW with automatic device detection, genre-based effects, and photorealistic UI.

## Features

### Core Functionality
- **Direct USB Recording**: Seamless recording from USB instruments and microphones
- **Auto-Detection**: Automatic identification of connected USB audio devices
- **Instrument Profiling**: Recognition of common instruments (guitars, mics, keyboards) with optimized settings
- **Low-Latency Processing**: Optimized for real-time performance with minimal latency

### Audio Effects
- **Genre-Based Effects**: Automatic effect selection based on musical genre
  - Rock: Distortion + Compressor + Reverb
  - Jazz: Clean compression + warm reverb
  - Metal: High-gain distortion + tight compression
  - Blues: Vintage overdrive + subtle effects
  - Electronic: Modern processing + spatial effects
  - And more...

- **Effect Chain**:
  - 3-Band EQ with adjustable frequency bands
  - Distortion/Overdrive with drive, tone, and level controls
  - Compressor with threshold, ratio, attack, release
  - Reverb with room size, damping, and wet/dry mix

### User Interface
- **Photorealistic Design**: Modern, intuitive interface with visual feedback
- **Real-time Monitoring**: CPU usage, latency, and device status indicators
- **Genre Selection**: Easy switching between musical styles
- **Effect Mix Control**: Blend between dry and processed signal

### Preset Management
- **Factory Presets**: Professional presets for each genre
- **Custom Presets**: Save and recall your own settings
- **Preset Categories**: Organized by genre and instrument type
- **Auto-save**: Automatic backup of settings

### Advanced Features
- **Live A/B Testing**: Compare different effect settings in real-time
- **Rare Instrument Profiles**: Extended database of specialized instruments
- **DAW Integration**: Seamless integration with Titan7 DAW
- **Multi-device Support**: Handle multiple USB devices simultaneously

## System Requirements

- **Operating System**: Windows 10+, macOS 10.14+, Linux (Ubuntu 18.04+)
- **Audio**: VST3 compatible DAW (Titan7 DAW recommended)
- **USB**: USB 2.0 or higher for audio device connectivity
- **Memory**: 512MB RAM minimum, 1GB recommended
- **CPU**: Intel/AMD dual-core processor minimum

## Installation

### Dependencies
The plugin requires the following libraries:
- PortAudio (for audio I/O)
- libusb (for USB device detection)
- VST3 SDK (for plugin framework)
- VSTGUI (for user interface)

### Building from Source

1. Install dependencies:
```bash
npm run install-deps
```

2. Build the plugin:
```bash
npm run build
```

3. The built plugin will be available in the `build/` directory.

## Usage

### Basic Setup
1. Launch your DAW (Titan7 recommended)
2. Load the Warrior USB Recorder plugin on an audio track
3. Connect your USB instrument or microphone
4. The plugin will automatically detect and configure the device

### Recording Process
1. **Device Detection**: Plugin automatically identifies connected USB devices
2. **Instrument Recognition**: Analyzes device characteristics to determine instrument type
3. **Genre Selection**: Choose appropriate genre or let auto-detection decide
4. **Effect Processing**: Real-time effects applied based on genre and instrument
5. **Recording**: Hit record in your DAW to capture the processed audio

### Parameters

#### Main Controls
- **Input Gain**: Adjust input level from USB device (0-100%)
- **Output Gain**: Control final output level (0-100%)
- **Genre**: Select musical style for automatic effect configuration
- **Effect Mix**: Blend between dry and processed signal (0-100%)

#### USB Controls
- **Auto-Detect**: Enable/disable automatic device detection
- **Record Enable**: Arm the plugin for recording
- **Device Status**: Visual indicator of USB device connection

#### Advanced Settings
- **Low-Latency Mode**: Optimize for minimal processing delay
- **Buffer Size**: Adjust for latency vs. stability balance
- **CPU Optimization**: Multiple performance levels available

## Supported Genres

- **Rock**: Classic rock distortion with moderate compression
- **Jazz**: Clean, warm tone with subtle compression and reverb
- **Blues**: Vintage overdrive with organic compression
- **Metal**: High-gain distortion with tight compression and EQ
- **Electronic**: Modern processing with spatial effects
- **Classical**: Clean recording with natural reverb
- **Country**: Bright, clear tone with light compression
- **Funk**: Punchy compression with tight EQ
- **Reggae**: Clean tone with spacious reverb
- **Pop**: Polished sound with balanced processing
- **Hip-Hop**: Clean recording with optional effects
- **Folk**: Natural, unprocessed sound

## Technical Specifications

- **Latency**: As low as 2.9ms (64-sample buffer at 48kHz)
- **Sample Rates**: 44.1kHz, 48kHz, 88.2kHz, 96kHz, 192kHz
- **Bit Depth**: 16, 24, 32-bit integer and 32-bit float
- **Channels**: Mono and stereo input/output
- **CPU Usage**: Typically 5-15% on modern systems
- **Plugin Format**: VST3

## Troubleshooting

### Common Issues

**Device Not Detected**:
- Ensure USB device is properly connected
- Check device compatibility with system audio drivers
- Try different USB ports
- Restart plugin or DAW if necessary

**High Latency**:
- Enable Low-Latency Mode
- Reduce buffer size in plugin settings
- Close other CPU-intensive applications
- Use dedicated USB ports (avoid hubs when possible)

**Audio Dropouts**:
- Increase buffer size
- Check CPU usage
- Ensure stable USB connection
- Update audio drivers

## Development

### Architecture
The plugin is built using modern C++ with the VST3 SDK and follows these design principles:
- **Modular Design**: Separate components for USB detection, effects, UI, and presets
- **Real-time Safety**: Lock-free audio processing with minimal allocations
- **Cross-platform**: Compatible with Windows, macOS, and Linux
- **Extensible**: Easy to add new effects, genres, and instrument profiles

### Key Components
- `USBAudioDetector`: Handles device discovery and monitoring
- `GenreEffectsEngine`: Manages effect chains and processing
- `LowLatencyProcessor`: Optimizes for real-time performance
- `PresetManager`: Handles preset storage and retrieval
- `WarriorUI`: Photorealistic user interface implementation

## License

MIT License - see LICENSE file for details.

## Support

For technical support, feature requests, or bug reports:
- Website: https://www.warrior-audio.com
- Email: support@warrior-audio.com
- GitHub: https://github.com/RJF-72/WarriorPlug-Ins

## Version History

### v1.0.0 (Current)
- Initial release
- USB auto-detection and device profiling
- 12 genre-based effect presets
- Photorealistic UI with real-time monitoring
- Low-latency processing engine
- Comprehensive preset management system
- Cross-platform VST3 plugin