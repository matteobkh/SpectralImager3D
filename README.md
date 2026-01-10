# Building SpectralImager3D on macOS 12.7.6

## Prerequisites
- macOS 12.7.6
- Xcode (install from App Store, or Xcode Command Line Tools)
- CMake installed (`brew install cmake` or from cmake.org)
- JUCE downloaded from https://github.com/juce-framework/JUCE

## Step-by-Step Build Instructions

### Step 1: Install Xcode Command Line Tools (if not already installed)
```bash
xcode-select --install
```

### Step 2: Set Up Directory Structure
Create a working directory and organize files:
```bash
# Create project directory
mkdir -p ~/AudioPlugins/SpectralImager3D
cd ~/AudioPlugins/SpectralImager3D

# Copy/move your JUCE folder here (or note its location)
# Example: ~/AudioPlugins/JUCE
```

### Step 3: Extract the Plugin Source
Extract `SpectralImager3D.zip` into your project folder:
```bash
cd ~/AudioPlugins/SpectralImager3D
unzip /path/to/SpectralImager3D.zip -d .
```

OR if JUCE is in a different location:
```cmake
add_subdirectory(/path/to/your/JUCE JUCE_build)
```

### Step 5: Build with CMake
```bash
cd ~/AudioPlugins/SpectralImager3D

# Configure the build (generates Xcode project or Makefiles)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build the plugin
cmake --build build --config Release
```

### Step 6: Find Your Built Plugin
After successful build, find your plugins in:
```bash
# VST3 plugin
ls build/SpectralImager3D_artefacts/Release/VST3/

# AU plugin (Audio Unit)
ls build/SpectralImager3D_artefacts/Release/AU/

# Standalone app
ls build/SpectralImager3D_artefacts/Release/Standalone/
```

### Step 7: Install the Plugin
Copy to your system plugin folders:
```bash
# VST3
cp -r build/SpectralImager3D_artefacts/Release/VST3/SpectralImager3D.vst3 ~/Library/Audio/Plug-Ins/VST3/

# AU (Audio Unit)
cp -r build/SpectralImager3D_artefacts/Release/AU/SpectralImager3D.component ~/Library/Audio/Plug-Ins/Components/
```

Then rescan plugins in your DAW.

---

## Troubleshooting

### Error: "Could not find JUCE"
Make sure you've edited CMakeLists.txt to point to your JUCE folder using `add_subdirectory()`.

### Error: "No CMAKE_CXX_COMPILER could be found"
Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### Error: OpenGL related errors
macOS 12 still supports OpenGL. Make sure Xcode is fully installed (not just command line tools) if you see framework errors.

### Error: Code signing issues
For local development, add this to CMakeLists.txt before `juce_add_plugin`:
```cmake
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
```

### Want to use Xcode IDE instead of command line?
Generate an Xcode project:
```bash
cmake -B build -G Xcode
open build/SpectralImager3D.xcodeproj
```
Then build from within Xcode (Product → Build).

---

## Quick Reference Commands

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
