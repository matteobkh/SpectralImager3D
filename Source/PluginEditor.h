/*
  ==============================================================================
    PluginEditor.h
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OpenGLRenderer.h"

class HSBColorPicker : public juce::Component
{
public:
    HSBColorPicker();
    void paint(juce::Graphics&) override;
    void resized() override;
    void setColor(juce::Colour c);
    juce::Colour getColor() const { return color; }
    std::function<void(float, float, float)> onChanged;
    
private:
    juce::Slider hue, sat, bri;
    juce::Colour color{ juce::Colours::cyan };
    void update();
};

class TrackList : public juce::Component, private juce::Timer
{
public:
    explicit TrackList(ITrackDataProvider& d);
    ~TrackList() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    ITrackDataProvider& data;
    int count = 0;
};

class SpectralImagerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit SpectralImagerAudioProcessorEditor(SpectralImagerAudioProcessor&);
    ~SpectralImagerAudioProcessorEditor() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    void timerCallback() override;
    void updateUI();
    
    SpectralImagerAudioProcessor& proc;
    
    juce::Label title;
    juce::ComboBox modeBox;
    
    // Sender UI
    HSBColorPicker colorPicker;
    juce::Label statusLbl;
    
    // Receiver UI - created lazily to avoid crash
    std::unique_ptr<Spectral3DRenderer> renderer;
    std::unique_ptr<TrackList> trackList;
    juce::ComboBox viewBox;
    juce::TextButton resetBtn{ "Reset View" };
    juce::Slider rangeSlider;
    juce::Label rangeLabel;
    juce::ToggleButton highResBtn{ "High Res" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> highResAttachment;
    
    bool uiInitialized = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectralImagerAudioProcessorEditor)
};
