/*
  ==============================================================================
    JuceHeader.h
  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>

#ifndef JucePlugin_Name
#define JucePlugin_Name "SpectralImager3D"
#endif
#ifndef JucePlugin_Desc
#define JucePlugin_Desc "3D Multi-Instance Stereo Spectrum Analyzer"
#endif
#ifndef JucePlugin_Manufacturer
#define JucePlugin_Manufacturer "AudioDev"
#endif
#ifndef JucePlugin_ManufacturerCode
#define JucePlugin_ManufacturerCode 0x41756478
#endif
#ifndef JucePlugin_PluginCode
#define JucePlugin_PluginCode 0x53493344
#endif
#ifndef JucePlugin_IsSynth
#define JucePlugin_IsSynth 0
#endif
#ifndef JucePlugin_WantsMidiInput
#define JucePlugin_WantsMidiInput 0
#endif
#ifndef JucePlugin_ProducesMidiOutput
#define JucePlugin_ProducesMidiOutput 0
#endif
#ifndef JucePlugin_IsMidiEffect
#define JucePlugin_IsMidiEffect 0
#endif
#ifndef JucePlugin_EditorRequiresKeyboardFocus
#define JucePlugin_EditorRequiresKeyboardFocus 0
#endif
#ifndef JucePlugin_Version
#define JucePlugin_Version 1.0.0
#endif
#ifndef JucePlugin_VersionCode
#define JucePlugin_VersionCode 0x10000
#endif
#ifndef JucePlugin_VersionString
#define JucePlugin_VersionString "1.0.0"
#endif
#ifndef JucePlugin_VSTUniqueID
#define JucePlugin_VSTUniqueID JucePlugin_PluginCode
#endif
#ifndef JucePlugin_VSTCategory
#define JucePlugin_VSTCategory kPlugCategAnalysis
#endif
#ifndef JucePlugin_Vst3Category
#define JucePlugin_Vst3Category "Analyzer"
#endif
#ifndef JucePlugin_AUMainType
#define JucePlugin_AUMainType 'aufx'
#endif
#ifndef JucePlugin_AUSubType
#define JucePlugin_AUSubType JucePlugin_PluginCode
#endif
#ifndef JucePlugin_AUExportPrefix
#define JucePlugin_AUExportPrefix SpectralImager3DAU
#endif
#ifndef JucePlugin_AUExportPrefixQuoted
#define JucePlugin_AUExportPrefixQuoted "SpectralImager3DAU"
#endif
#ifndef JucePlugin_AUManufacturerCode
#define JucePlugin_AUManufacturerCode JucePlugin_ManufacturerCode
#endif
#ifndef JucePlugin_CFBundleIdentifier
#define JucePlugin_CFBundleIdentifier com.audiodev.spectralimager3d
#endif
#ifndef JucePlugin_AAXIdentifier
#define JucePlugin_AAXIdentifier com.audiodev.spectralimager3d
#endif
#ifndef JucePlugin_AAXManufacturerCode
#define JucePlugin_AAXManufacturerCode JucePlugin_ManufacturerCode
#endif
#ifndef JucePlugin_AAXProductId
#define JucePlugin_AAXProductId JucePlugin_PluginCode
#endif
#ifndef JucePlugin_AAXCategory
#define JucePlugin_AAXCategory 0
#endif
#ifndef JucePlugin_AAXDisableBypass
#define JucePlugin_AAXDisableBypass 0
#endif
#ifndef JucePlugin_AAXDisableMultiMono
#define JucePlugin_AAXDisableMultiMono 0
#endif
#ifndef JucePlugin_MaxNumInputChannels
#define JucePlugin_MaxNumInputChannels 2
#endif
#ifndef JucePlugin_MaxNumOutputChannels
#define JucePlugin_MaxNumOutputChannels 2
#endif
#ifndef JucePlugin_PreferredChannelConfigurations
#define JucePlugin_PreferredChannelConfigurations {2, 2}
#endif

using namespace juce;
