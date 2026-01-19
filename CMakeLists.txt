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
# JUCE CONFIGURATION
# =============================================================================
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/JUCE ${CMAKE_BINARY_DIR}/JUCE_build)

# Common Source Files
set(SourceFiles
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SharedDataManager.h
    Source/SpectralAnalyzer.h
    Source/OpenGLRenderer.h
)

# ==============================================================================
# Target 1: Standard Stereo Sender/Receiver
# ==============================================================================
juce_add_plugin(SpectralImager3D
    COMPANY_NAME "Arcane Machines"
    BUNDLE_ID "com.arcanemachines.spectralimager3d"
    PLUGIN_MANUFACTURER_CODE Arcn
    PLUGIN_CODE SI3D
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_NAME "SpectralImager3D"
    PRODUCT_NAME "SpectralImager3D"
    DESCRIPTION "3D Multi-Instance Stereo/Spectral Imager"
    VST3_CATEGORIES Analyzer Tools
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(SpectralImager3D)
target_sources(SpectralImager3D PRIVATE ${SourceFiles})
target_compile_definitions(SpectralImager3D PUBLIC 
    JUCE_WEB_BROWSER=0 
    JUCE_USE_CURL=0 
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(SpectralImager3D PRIVATE
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
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ==============================================================================
# Target 2: 16-Channel Unified Receiver
# ==============================================================================
juce_add_plugin(SpectralImager3D_16Ch
    COMPANY_NAME "Arcane Machines"
    BUNDLE_ID "com.arcanemachines.spectralimager3d16ch"
    PLUGIN_MANUFACTURER_CODE Arcn
    PLUGIN_CODE SI16
    FORMATS VST3 AU Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_NAME "SpectralImager3D 16Ch"
    PRODUCT_NAME "SpectralImager3D 16Ch"
    DESCRIPTION "16-Channel 3D Spectral Imager"
    VST3_CATEGORIES Analyzer Tools
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(SpectralImager3D_16Ch)
target_sources(SpectralImager3D_16Ch PRIVATE ${SourceFiles})
target_compile_definitions(SpectralImager3D_16Ch PUBLIC 
    JUCE_WEB_BROWSER=0 
    JUCE_USE_CURL=0 
    JUCE_VST3_CAN_REPLACE_VST2=0
    SI3D_16CH_UNIFIED=1
)
target_link_libraries(SpectralImager3D_16Ch PRIVATE
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
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# Platform-specific settings
if(APPLE)
    target_link_libraries(SpectralImager3D PRIVATE "-framework OpenGL" "-framework CoreAudio" "-framework CoreMIDI" "-framework Accelerate")
    target_link_libraries(SpectralImager3D_16Ch PRIVATE "-framework OpenGL" "-framework CoreAudio" "-framework CoreMIDI" "-framework Accelerate")
elseif(WIN32)
    target_link_libraries(SpectralImager3D PRIVATE opengl32)
    target_link_libraries(SpectralImager3D_16Ch PRIVATE opengl32)
elseif(UNIX)
    find_package(OpenGL REQUIRED)
    target_link_libraries(SpectralImager3D PRIVATE OpenGL::GL)
    target_link_libraries(SpectralImager3D_16Ch PRIVATE OpenGL::GL)
endif()
