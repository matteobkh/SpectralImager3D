# Spectral Imager 3D

## Building SpectralImager3D on macOS

### Prerequisites
- macOS 12.7.6
- Xcode (install from App Store, or Xcode Command Line Tools)
- CMake installed (`brew install cmake` or from cmake.org)

### Quick Reference Commands

```bash
# Full clean rebuild
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Generate Xcode project
cmake -B build -G Xcode

# Build only VST3
cmake --build build --target SpectralImager3D_VST3 --config Release

# Build only AU
cmake --build build --target SpectralImager3D_AU --config Release
```

### Find Your Built Plugin
After successful build, find your plugins in:
```bash
# VST3 plugin
ls build/SpectralImager3D_artefacts/Release/VST3/

# AU plugin (Audio Unit)
ls build/SpectralImager3D_artefacts/Release/AU/

# Standalone app
ls build/SpectralImager3D_artefacts/Release/Standalone/
```

### Install the Plugin
Copy to your system plugin folders:
```bash
# VST3
cp -r build/SpectralImager3D_artefacts/Release/VST3/SpectralImager3D.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (Audio Unit)
cp -r build/SpectralImager3D_artefacts/Release/AU/SpectralImager3D.component ~/Library/Audio/Plug-Ins/Components/
```