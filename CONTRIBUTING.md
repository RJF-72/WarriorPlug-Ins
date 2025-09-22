# Contributing to WarriorPlug-Ins

We welcome contributions to the WarriorPlug-Ins project! Whether you're fixing bugs, adding features, or improving documentation, your help is appreciated.

## Development Setup

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/yourusername/WarriorPlug-Ins.git
   cd WarriorPlug-Ins
   ```

3. **Install dependencies**:
   - JUCE Framework (automatically fetched by CMake)
   - CMake 3.15+
   - C++17 compatible compiler

4. **Build the project**:
   ```bash
   ./build.sh
   ```

## Code Style

- Use **4 spaces** for indentation (no tabs)
- Follow **JUCE naming conventions**:
  - Class names: `PascalCase`
  - Member variables: `camelCase`
  - Constants: `UPPER_CASE`
- Keep lines under **120 characters**
- Use **meaningful variable names**

## Project Structure

```
src/
├── common/                 # Shared utilities
│   ├── DSPUtils.h/cpp     # DSP algorithms and utilities
│   └── WarriorLookAndFeel.h/cpp  # UI theme and styling
├── WarriorReverb/         # Reverb plugin
├── WarriorDelay/          # Delay plugin
├── WarriorDistortion/     # Distortion plugin
└── WarriorCompressor/     # Compressor plugin
```

## Adding New Features

### For existing plugins:
1. Add parameters to the `createParameterLayout()` function
2. Update the processor's `processBlock()` method
3. Add UI controls to the editor
4. Update documentation

### For new plugins:
1. Create a new directory under `src/`
2. Implement `PluginProcessor.h/cpp` and `PluginEditor.h/cpp`
3. Add the plugin to `CMakeLists.txt`
4. Update documentation

## DSP Guidelines

- **Use the shared DSP utilities** in `src/common/DSPUtils.h`
- **Optimize for real-time performance**: avoid allocations in `processBlock()`
- **Use JUCE's built-in functions** when possible
- **Test with different sample rates** (44.1kHz, 48kHz, 96kHz)
- **Implement proper parameter smoothing** to avoid clicks

## UI Guidelines

- **Use the WarriorLookAndFeel** for consistent styling
- **Follow the warrior theme**: dark colors with orange/blue accents
- **Make controls intuitive**: clear labels and logical grouping
- **Support different screen sizes**: test on various resolutions
- **Implement proper parameter binding** with AudioProcessorValueTreeState

## Testing

Before submitting a pull request:

1. **Build successfully** on your platform
2. **Test all plugin formats** (VST3, AU, Standalone)
3. **Test parameter automation** in a DAW
4. **Check for audio artifacts** (clicks, pops, distortion)
5. **Verify preset saving/loading** works correctly

## Submitting Changes

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** with clear, descriptive commits:
   ```bash
   git commit -m "Add vintage tube saturation to WarriorDistortion"
   ```

3. **Update documentation** if needed

4. **Test thoroughly** on your platform

5. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create a Pull Request** on GitHub with:
   - Clear description of changes
   - Screenshots/audio examples if relevant
   - Testing information

## Bug Reports

When reporting bugs, please include:

- **Operating system** and version
- **DAW** name and version (if applicable)
- **Plugin format** (VST3, AU, Standalone)
- **Steps to reproduce** the issue
- **Expected vs actual behavior**
- **Audio examples** if relevant

## Feature Requests

For feature requests:

- **Describe the use case** clearly
- **Explain why** it would be valuable
- **Consider implementation complexity**
- **Provide examples** from other plugins if relevant

## Getting Help

- **Open an issue** on GitHub for questions
- **Check existing issues** before creating new ones
- **Be specific** about what you need help with

## License

By contributing, you agree that your contributions will be licensed under the GPL v3.0 license that covers the project.

Thank you for contributing to WarriorPlug-Ins!