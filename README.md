# Spectral Imager 3D
Visualize a mix in 3D, see where different elements overlap in the frequency spectrum and the stereo image at the same time. Use all the space in your mix and keep different sounds clear separate. Apply an instance in "Sender" mode to any track and see it represented in the 3D analyzer on a "Receiver" instance anywhere in your project.
## Receiver: viewing modes
**3D Perspective view**
<img width="646" height="672" alt="Screen Shot 2026-01-11 at 2 44 01 PM" src="https://github.com/user-attachments/assets/375b49cf-db16-4dae-81de-0b27de06fa6c" />

* Mouse wheel to zoom in and out
* Click and drag to move the camera
**Flat top down view**
<img width="646" height="672" alt="Screen Shot 2026-01-11 at 2 44 12 PM" src="https://github.com/user-attachments/assets/999a1f91-8e68-4628-868d-78d2bcd361ee" />

* View stereo width and placement across the frequency spectrum
**Flat side view**
<img width="646" height="672" alt="Screen Shot 2026-01-11 at 2 44 32 PM" src="https://github.com/user-attachments/assets/e250d982-0998-428d-babe-a5d5e557377e" />

* View the frequency spectrum and how your sounds may overlap
## Sender
<img width="646" height="515" alt="Screen Shot 2026-01-11 at 2 44 51 PM" src="https://github.com/user-attachments/assets/ab265574-3a91-4d2a-8b12-f169bf5d417c" />
* Choose track color, a random hue is selected for each new instance loaded
* High Res mode, off: 24 bands (fast), on: 48 bands (accurate)

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
