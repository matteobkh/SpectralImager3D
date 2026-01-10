/*
  ==============================================================================
    PluginProcessor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralImagerAudioProcessor::SpectralImagerAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Params", createParams())
{
    instId = reinterpret_cast<uint64_t>(this);
    slot = sharedData->registerSender(instId);
    if (slot >= 0) sharedData->getTrack(slot).setColor(color);
    
    apvts.addParameterListener("mode", this);
    apvts.addParameterListener("hue", this);
    apvts.addParameterListener("sat", this);
    apvts.addParameterListener("bri", this);
    apvts.addParameterListener("range", this);
    apvts.addParameterListener("highres", this);
}

SpectralImagerAudioProcessor::~SpectralImagerAudioProcessor()
{
    apvts.removeParameterListener("mode", this);
    apvts.removeParameterListener("hue", this);
    apvts.removeParameterListener("sat", this);
    apvts.removeParameterListener("bri", this);
    apvts.removeParameterListener("range", this);
    apvts.removeParameterListener("highres", this);
    sharedData->unregisterSender(instId);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralImagerAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", juce::StringArray{"Sender", "Receiver"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("hue", "Hue", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("sat", "Saturation", 0.0f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("bri", "Brightness", 0.0f, 1.0f, 0.9f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "range", "Range (dB)", 
        juce::NormalisableRange<float>(12.0f, 60.0f, 1.0f), 
        36.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("highres", "High Resolution", false));
    return { p.begin(), p.end() };
}

void SpectralImagerAudioProcessor::parameterChanged(const juce::String& id, float val)
{
    if (id == "mode")
    {
        setMode(val < 0.5f ? PluginMode::Sender : PluginMode::Receiver);
    }
    else if (id == "hue" || id == "sat" || id == "bri")
    {
        setTrackColor(juce::Colour::fromHSV(apvts.getRawParameterValue("hue")->load(),
                                            apvts.getRawParameterValue("sat")->load(),
                                            apvts.getRawParameterValue("bri")->load(), 1.0f));
    }
    else if (id == "range")
    {
        setRange(val);
    }
    else if (id == "highres")
    {
        int newBands = val > 0.5f ? 48 : 24;
        setNumBands(newBands);
        analyzer.setNumBands(newBands);
    }
}

void SpectralImagerAudioProcessor::setMode(PluginMode m)
{
    if (mode == m) return;
    mode = m;
    
    if (m == PluginMode::Sender)
    {
        if (slot < 0)
        {
            slot = sharedData->registerSender(instId);
            if (slot >= 0) sharedData->getTrack(slot).setColor(color);
        }
    }
    else
    {
        if (slot >= 0)
        {
            sharedData->unregisterSender(instId);
            slot = -1;
        }
    }
}

void SpectralImagerAudioProcessor::setTrackColor(juce::Colour c)
{
    color = c;
    if (slot >= 0) sharedData->getTrack(slot).setColor(c);
}

void SpectralImagerAudioProcessor::prepareToPlay(double sr, int block)
{
    // Sync analyzer band count with parameter
    bool highRes = apvts.getRawParameterValue("highres")->load() > 0.5f;
    int bands = highRes ? 48 : 24;
    numBands = bands;
    analyzer.setNumBands(bands);
    analyzer.prepare(sr, block);
}

void SpectralImagerAudioProcessor::releaseResources() { analyzer.clear(); }

bool SpectralImagerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpectralImagerAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buf.clear(i, 0, buf.getNumSamples());
    
    if (mode != PluginMode::Sender || slot < 0) return;
    
    const float* L = buf.getReadPointer(0);
    const float* R = buf.getNumChannels() > 1 ? buf.getReadPointer(1) : L;
    
    if (analyzer.process(L, R, buf.getNumSamples()))
    {
        auto& track = sharedData->getTrack(slot);
        const auto& res = analyzer.getResults();
        int bands = analyzer.getNumBands();
        track.numBands.store(bands, std::memory_order_relaxed);
        for (int i = 0; i < bands; ++i)
            track.setBand(static_cast<size_t>(i), res[static_cast<size_t>(i)].leftLevel, 
                         res[static_cast<size_t>(i)].rightLevel);
        sharedData->updateTimestamp(slot);
    }
}

void SpectralImagerAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void SpectralImagerAudioProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
        
        float modeVal = apvts.getRawParameterValue("mode")->load();
        setMode(modeVal < 0.5f ? PluginMode::Sender : PluginMode::Receiver);
        setTrackColor(juce::Colour::fromHSV(apvts.getRawParameterValue("hue")->load(),
                                            apvts.getRawParameterValue("sat")->load(),
                                            apvts.getRawParameterValue("bri")->load(), 1.0f));
    }
}

juce::AudioProcessorEditor* SpectralImagerAudioProcessor::createEditor()
{
    return new SpectralImagerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectralImagerAudioProcessor(); }
