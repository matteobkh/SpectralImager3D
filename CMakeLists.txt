# ==============================================================================
# CMakeLists.txt
# 3D Multi-Instance Stereo/Spectral Imager
# ==============================================================================

cmake_minimum_required(VERSION 3.22)
project(SpectralImager3D VERSION 1.0.0)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable code signing for local development on macOS
if(APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
endif()

# =============================================================================
# JUCE CONFIGURATION - EDIT THIS PATH TO MATCH YOUR JUCE LOCATION
# =============================================================================
# Option 1: JUCE is a sibling folder (recommended setup)
# Your folder structure should be:
#   ~/AudioPlugins/JUCE/
#   ~/AudioPlugins/SpectralImager3D/  (this project)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE ${CMAKE_BINARY_DIR}/JUCE_build)

# Option 2: If your JUCE is elsewhere, comment out Option 1 and use this instead:
# add_subdirectory(/Users/YOUR_USERNAME/path/to/JUCE ${CMAKE_BINARY_DIR}/JUCE_build)
# =============================================================================

# Plugin target
juce_add_plugin(SpectralImager3D
    # Plugin info
    COMPANY_NAME "AudioDev"
    BUNDLE_ID "com.audiodev.spectralimager3d"
    PLUGIN_MANUFACTURER_CODE Audx
    PLUGIN_CODE SI3D
    
    # Formats
    FORMATS VST3 AU Standalone
    
    # Plugin type
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    
    # Editor
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    
    # Plugin name
    PLUGIN_NAME "SpectralImager3D"
    PRODUCT_NAME "SpectralImager3D"
    
    # Description
    DESCRIPTION "3D Multi-Instance Stereo/Spectral Imager"
    
    # VST3 category
    VST3_CATEGORIES Analyzer Tools
    
    # AU type
    AU_MAIN_TYPE kAudioUnitType_Effect
)

# Generate JUCE header
juce_generate_juce_header(SpectralImager3D)

# Source files
target_sources(SpectralImager3D
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SharedDataManager.h
        Source/SpectralAnalyzer.h
        Source/OpenGLRenderer.h
)

# Preprocessor definitions
target_compile_definitions(SpectralImager3D
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link JUCE modules
target_link_libraries(SpectralImager3D
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Platform-specific settings
if(APPLE)
    # macOS/iOS frameworks
    target_link_libraries(SpectralImager3D
        PRIVATE
            "-framework OpenGL"
            "-framework CoreAudio"
            "-framework CoreMIDI"
            "-framework Accelerate"
    )
elseif(WIN32)
    # Windows libraries
    target_link_libraries(SpectralImager3D
        PRIVATE
            opengl32
    )
elseif(UNIX)
    # Linux packages
    find_package(OpenGL REQUIRED)
    target_link_libraries(SpectralImager3D
        PRIVATE
            OpenGL::GL
    )
endif()
