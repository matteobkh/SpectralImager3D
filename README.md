# Spectral Imager 3D
Visualize a mix in 3D, see where different elements overlap in the frequency spectrum and the stereo image at the same time. Use all the space in your mix and keep different sounds clear separate. Apply an instance in "Sender" mode to any track and see it represented in the 3D analyzer on a "Receiver" instance anywhere in your project.

## Multichannel and stereo versions
The plugin can be deployed as a 16 in / 2 out version created for DAWs like Reaper that have multichannel capabilites, or as a basic stereo plugin for DAWs like Ableton without them. To compensate it has sender and receiver modes for sending up to 16 distinct stereo tracks to the visualizer.

### Using the 16ch version
In a DAW that supports multi-channel audio (like Reaper), route stereo tracks to a multi-channel bus using inputs 1&2, 2&3, ..., up to 15&16 for a total of 8 stereo tracks. Load "SpectralImager3D 16Ch" on the bus.

### Using the stereo version
Load "SpectralImager3D" on each individual stereo track, up to 16 of them. It will automatically be loaded in "Sender" mode. More about the sender mode below. Then load an instance anywhere in your project, on any track, and use the drop-down menu at the top of the GUI to select "Receiver" mode.
#### Sender
<img width="646" height="515" alt="Screen Shot 2026-01-11 at 2 44 51 PM" src="https://github.com/user-attachments/assets/ab265574-3a91-4d2a-8b12-f169bf5d417c" />

* Choose track color, a random hue is selected for each new instance loaded
* High Res mode, off: 24 bands (fast), on: 48 bands (accurate)


## Viewing modes
**3D Perspective view**

<img width="758" height="865" alt="Screen Shot 2026-01-19 at 6 19 31 PM" src="https://github.com/user-attachments/assets/242cfb65-683e-4ef3-808f-499b5b1e98c5" />


* Mouse wheel to zoom in and out
* Click and drag to move the camera
* X axis: stereo image, Y axis: amplitude, Z axis: frequency

**Flat top down view**
  
<img width="646" height="672" alt="Screen Shot 2026-01-11 at 2 44 12 PM" src="https://github.com/user-attachments/assets/999a1f91-8e68-4628-868d-78d2bcd361ee" />

* X axis: stereo image, Y axis: frequency

**Flat side view**
  
<img width="646" height="672" alt="Screen Shot 2026-01-11 at 2 44 32 PM" src="https://github.com/user-attachments/assets/e250d982-0998-428d-babe-a5d5e557377e" />

* View the frequency spectrum and how your sounds may overlap
* X axis: frequency, Y axis: amplitude

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
```

### Install the Plugin
Copy to your system plugin folders:
```bash
# VST3
cp -r build/SpectralImager3D_artefacts/Release/VST3/SpectralImager3D.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (Audio Unit)
cp -r build/SpectralImager3D_artefacts/Release/AU/SpectralImager3D.component ~/Library/Audio/Plug-Ins/Components/
```
