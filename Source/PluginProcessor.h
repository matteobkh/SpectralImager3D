/*
  ==============================================================================
    PluginProcessor.h
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "SharedDataManager.h"
#include "SpectralAnalyzer.h"

enum class PluginMode { Sender, Receiver };

class SpectralImagerAudioProcessor : public juce::AudioProcessor,
                                     public juce::AudioProcessorValueTreeState::Listener
{
public:
    SpectralImagerAudioProcessor();
    ~SpectralImagerAudioProcessor() override;
    
    void prepareToPlay(double sr, int block) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    
    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int size) override;
    
    void parameterChanged(const juce::String& id, float val) override;
    
    PluginMode getMode() const { return mode; }
    void setMode(PluginMode m);
    juce::Colour getTrackColor() const { return color; }
    void setTrackColor(juce::Colour c);
    float getRange() const { return range; }
    void setRange(float r) { range = juce::jlimit(12.0f, 90.0f, r); }
    int getNumBands() const { return numBands; }
    void setNumBands(int n) { numBands = juce::jlimit(12, 64, n); 
#ifdef SI3D_16CH_UNIFIED
        for(auto& a : analyzers) a.setNumBands(numBands);
#else
        analyzer.setNumBands(numBands); 
#endif
    }
    
    ITrackDataProvider& getSharedData() { 
#ifdef SI3D_16CH_UNIFIED
        return sharedData;
#else
        return *sharedData; 
#endif
    }
    int getSlot() const { return slot; }
    
    juce::AudioProcessorValueTreeState apvts;
    
private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    
#ifdef SI3D_16CH_UNIFIED
    LocalDataManager sharedData;
    std::array<SpectralAnalyzer, 8> analyzers;
#else
    juce::SharedResourcePointer<SharedDataManager> sharedData;
    SpectralAnalyzer analyzer;
#endif

    PluginMode mode = PluginMode::Sender;
    juce::Colour color{ 0xFF00FFFF };
    float range = 90.0f;
    int numBands = 24;
    int slot = -1;
    uint64_t instId = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralImagerAudioProcessor)
};
