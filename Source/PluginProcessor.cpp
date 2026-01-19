/*
  ==============================================================================
    PluginProcessor.cpp
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectralImagerAudioProcessor::SpectralImagerAudioProcessor()
#ifdef SI3D_16CH_UNIFIED
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::discreteChannels(16), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#else
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
      apvts(*this, nullptr, "Params", createParams())
{
    instId = reinterpret_cast<uint64_t>(this);

#ifdef SI3D_16CH_UNIFIED
    // Force Receiver mode for UI
    mode = PluginMode::Receiver;
    
    // Initialize 8 fixed tracks with rainbow colors
    for (int i = 0; i < 8; ++i)
    {
        auto& t = sharedData.getTrack(i);
        t.instanceId.store(instId + static_cast<uint64_t>(i));
        t.setColor(juce::Colour::fromHSV(static_cast<float>(i) / 8.0f, 0.85f, 1.0f, 1.0f));
    }
#else
    // Randomize Hue if this is a fresh instance (not yet loaded from state)
    auto* hueParam = apvts.getParameter("hue");
    float randomHue = juce::Random::getSystemRandom().nextFloat();
    hueParam->setValueNotifyingHost(randomHue);
    // Set the internal color variable immediately
    color = juce::Colour::fromHSV(randomHue, 0.8f, 1.0f, 1.0f);

    slot = sharedData->registerSender(instId);
    if (slot >= 0) sharedData->getTrack(slot).setColor(color);
    
    apvts.addParameterListener("mode", this);
    apvts.addParameterListener("hue", this);
    apvts.addParameterListener("sat", this);
    apvts.addParameterListener("bri", this);
#endif

    apvts.addParameterListener("range", this);
    apvts.addParameterListener("highres", this);
}

SpectralImagerAudioProcessor::~SpectralImagerAudioProcessor()
{
#ifndef SI3D_16CH_UNIFIED
    apvts.removeParameterListener("mode", this);
    apvts.removeParameterListener("hue", this);
    apvts.removeParameterListener("sat", this);
    apvts.removeParameterListener("bri", this);
    sharedData->unregisterSender(instId);
#endif
    apvts.removeParameterListener("range", this);
    apvts.removeParameterListener("highres", this);
}

juce::AudioProcessorValueTreeState::ParameterLayout SpectralImagerAudioProcessor::createParams()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    
#ifndef SI3D_16CH_UNIFIED
    p.push_back(std::make_unique<juce::AudioParameterChoice>("mode", "Mode", juce::StringArray{"Sender", "Receiver"}, 0));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("hue", "Hue", 0.0f, 1.0f, 0.5f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("sat", "Saturation", 0.0f, 1.0f, 0.8f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>("bri", "Brightness", 0.0f, 1.0f, 0.9f));
#endif

    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        "range", "Range (dB)", 
        juce::NormalisableRange<float>(12.0f, 90.0f, 1.0f), 
        90.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>("highres", "High Resolution", true));
    return { p.begin(), p.end() };
}

void SpectralImagerAudioProcessor::parameterChanged(const juce::String& id, float val)
{
#ifndef SI3D_16CH_UNIFIED
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
    else 
#endif
    if (id == "range")
    {
        setRange(val);
    }
    else if (id == "highres")
    {
        int newBands = val > 0.5f ? 48 : 24;
        setNumBands(newBands);
#ifndef SI3D_16CH_UNIFIED
        analyzer.setNumBands(newBands);
#endif
    }
}

#ifndef SI3D_16CH_UNIFIED
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
#endif

void SpectralImagerAudioProcessor::prepareToPlay(double sr, int block)
{
    // Sync analyzer band count with parameter
    bool highRes = apvts.getRawParameterValue("highres")->load() > 0.5f;
    int bands = highRes ? 48 : 24;
    numBands = bands;

#ifdef SI3D_16CH_UNIFIED
    for (auto& a : analyzers)
    {
        a.setNumBands(bands);
        a.prepare(sr, block);
    }
#else
    analyzer.setNumBands(bands);
    analyzer.prepare(sr, block);
#endif
}

void SpectralImagerAudioProcessor::releaseResources() 
{ 
#ifdef SI3D_16CH_UNIFIED
    for (auto& a : analyzers) a.clear();
#else
    analyzer.clear(); 
#endif
}

bool SpectralImagerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#ifdef SI3D_16CH_UNIFIED
    // Require explicit 16 in / Stereo out? Or allow any input > 0?
    // JUCE often negotiates. Let's strict check 16 inputs for now to match our logic, 
    // or allow fewer but map safely.
    // Ideally we want input channels == 16.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet().size() <= 16;
#else
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
#endif
}

void SpectralImagerAudioProcessor::processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    int totalIn = getTotalNumInputChannels();
    int totalOut = getTotalNumOutputChannels();
    int samples = buf.getNumSamples();

#ifdef SI3D_16CH_UNIFIED
    // Process up to 8 stereo pairs
    int pairs = std::min(8, (totalIn + 1) / 2);
    
    // 1. Analyze all pairs first (while input buffer is pristine)
    for (int i = 0; i < pairs; ++i)
    {
        // Get pointers for this pair
        int chL = i * 2;
        int chR = chL + 1;
        
        const float* pL = (chL < totalIn) ? buf.getReadPointer(chL) : nullptr;
        const float* pR = (chR < totalIn) ? buf.getReadPointer(chR) : pL;
        if (!pR) pR = pL; // Handle mono last channel
        
        if (!pL) continue;

        // Analyze
        if (analyzers[static_cast<size_t>(i)].process(pL, pR, samples))
        {
            auto& track = sharedData.getTrack(i);
            const auto& res = analyzers[static_cast<size_t>(i)].getResults();
            int bands = analyzers[static_cast<size_t>(i)].getNumBands();
            
            track.numBands.store(bands, std::memory_order_relaxed);
            for (int b = 0; b < bands; ++b)
                track.setBand(static_cast<size_t>(b), res[static_cast<size_t>(b)].leftLevel, 
                             res[static_cast<size_t>(b)].rightLevel);
            
            sharedData.updateTimestamp(i);
        }
    }
    
    // 2. Simple Mixdown to Output Stereo
    // Sum all pairs to channels 0 and 1
    if (totalOut >= 2 && totalIn >= 2)
    {
        float gain = 0.25f; // -12dB headroom
        
        // Scale Pair 0 (in place)
        buf.applyGain(0, 0, samples, gain);
        buf.applyGain(1, 0, samples, gain);
        
        // Add other pairs
        for (int i = 1; i < pairs; ++i)
        {
            int chL = i * 2;
            int chR = chL + 1;
            if (chL < totalIn) buf.addFrom(0, 0, buf, chL, 0, samples, gain);
            if (chR < totalIn) buf.addFrom(1, 0, buf, chR, 0, samples, gain);
        }
        
        // Clear any extra output channels
        for (int i = 2; i < totalOut; ++i)
            buf.clear(i, 0, samples);
    }
#else
    for (int i = totalIn; i < totalOut; ++i)
        buf.clear(i, 0, samples);
    
    if (mode != PluginMode::Sender || slot < 0) return;
    
    const float* L = buf.getReadPointer(0);
    const float* R = buf.getNumChannels() > 1 ? buf.getReadPointer(1) : L;
    
    if (analyzer.process(L, R, samples))
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
#endif
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
        
#ifndef SI3D_16CH_UNIFIED
        float modeVal = apvts.getRawParameterValue("mode")->load();
        setMode(modeVal < 0.5f ? PluginMode::Sender : PluginMode::Receiver);
        setTrackColor(juce::Colour::fromHSV(apvts.getRawParameterValue("hue")->load(),
                                            apvts.getRawParameterValue("sat")->load(),
                                            apvts.getRawParameterValue("bri")->load(), 1.0f));
#endif
    }
}

juce::AudioProcessorEditor* SpectralImagerAudioProcessor::createEditor()
{
    return new SpectralImagerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SpectralImagerAudioProcessor(); }
