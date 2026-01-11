/*
  ==============================================================================
    PluginEditor.cpp
  ==============================================================================
*/

#include "PluginEditor.h"

namespace UI
{
    const juce::Colour bg1(0xFF0d1117);
    const juce::Colour bg2(0xFF161b22);
    const juce::Colour panel(0xFF21262d);
    const juce::Colour border(0xFF30363d);
    const juce::Colour text(0xFFc9d1d9);
    const juce::Colour textDim(0xFF8b949e);
}

//==============================================================================
HSBColorPicker::HSBColorPicker()
{
    auto setupSlider = [this](juce::Slider& s, juce::Colour c) {
        s.setRange(0, 1, 0.01);
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::thumbColourId, c);
        s.setColour(juce::Slider::trackColourId, c.darker(0.5f));
        s.setColour(juce::Slider::backgroundColourId, UI::bg1);
        s.onValueChange = [this] { update(); };
        addAndMakeVisible(s);
    };
    
    setupSlider(hue, juce::Colours::red);
    setupSlider(sat, juce::Colours::grey);
    setupSlider(bri, juce::Colours::white);
    
    hue.setValue(0.5);
    sat.setValue(0.8);
    bri.setValue(0.9);
}

void HSBColorPicker::paint(juce::Graphics& g)
{
    auto b = getLocalBounds();
    auto preview = b.removeFromRight(50).reduced(4);
    
    g.setColour(color);
    g.fillRoundedRectangle(preview.toFloat(), 6.0f);
    g.setColour(UI::border);
    g.drawRoundedRectangle(preview.toFloat(), 6.0f, 1.0f);
    
    g.setColour(UI::textDim);
    g.setFont(juce::FontOptions(11.0f));
    int y = 2;
    for (const char* lbl : {"H", "S", "B"})
    {
        g.drawText(lbl, 0, y, 15, 20, juce::Justification::centredRight);
        y += 24;
    }
}

void HSBColorPicker::resized()
{
    auto b = getLocalBounds();
    b.removeFromRight(55);
    b.removeFromLeft(18);
    
    int h = b.getHeight() / 3;
    hue.setBounds(b.removeFromTop(h).reduced(2));
    sat.setBounds(b.removeFromTop(h).reduced(2));
    bri.setBounds(b.reduced(2));
}

void HSBColorPicker::setColor(juce::Colour c)
{
    color = c;
    hue.setValue(c.getHue(), juce::dontSendNotification);
    sat.setValue(c.getSaturation(), juce::dontSendNotification);
    bri.setValue(c.getBrightness(), juce::dontSendNotification);
    repaint();
}

void HSBColorPicker::update()
{
    color = juce::Colour::fromHSV(static_cast<float>(hue.getValue()),
                                   static_cast<float>(sat.getValue()),
                                   static_cast<float>(bri.getValue()), 1.0f);
    repaint();
    if (onChanged)
        onChanged(static_cast<float>(hue.getValue()),
                  static_cast<float>(sat.getValue()),
                  static_cast<float>(bri.getValue()));
}

//==============================================================================
TrackList::TrackList(juce::SharedResourcePointer<SharedDataManager>& d) : data(d)
{
    startTimerHz(10);
}

TrackList::~TrackList() { stopTimer(); }

void TrackList::paint(juce::Graphics& g)
{
    g.setColour(UI::panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    
    g.setColour(UI::text);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Tracks: " + juce::String(count) + "/" + juce::String(static_cast<int>(kMaxTracks)),
               10, 0, 100, getHeight(), juce::Justification::centredLeft);
    
    int x = 115;
    for (size_t i = 0; i < kMaxTracks && x < getWidth() - 20; ++i)
    {
        if (data->getTrack(static_cast<int>(i)).isActive.load())
        {
            g.setColour(data->getTrack(static_cast<int>(i)).getColor());
            g.fillEllipse(static_cast<float>(x), static_cast<float>(getHeight()/2 - 6), 12.0f, 12.0f);
            x += 16;
        }
    }
}

void TrackList::timerCallback()
{
    int n = data->getActiveCount();
    if (n != count) { count = n; repaint(); }
}

//==============================================================================
SpectralImagerAudioProcessorEditor::SpectralImagerAudioProcessorEditor(SpectralImagerAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    // Set size first
    setSize(600, 750);
    setResizable(true, true);
    setResizeLimits(600, 450, 1400, 1000);
    
    // Title
    title.setText("Spectral Imager 3D", juce::dontSendNotification);
    title.setFont(juce::FontOptions(20.0f));
    title.setColour(juce::Label::textColourId, UI::text);
    addAndMakeVisible(title);
    
    // Mode selector
    modeBox.addItem("Sender", 1);
    modeBox.addItem("Receiver", 2);
    modeBox.setColour(juce::ComboBox::backgroundColourId, UI::panel);
    modeBox.setColour(juce::ComboBox::textColourId, UI::text);
    modeBox.setColour(juce::ComboBox::outlineColourId, UI::border);
    modeBox.setSelectedId(proc.getMode() == PluginMode::Sender ? 1 : 2, juce::dontSendNotification);
    modeBox.onChange = [this] {
        int selected = modeBox.getSelectedId();
        if (selected == 1)
            proc.apvts.getParameter("mode")->setValueNotifyingHost(0.0f);
        else if (selected == 2)
            proc.apvts.getParameter("mode")->setValueNotifyingHost(1.0f);
        
        juce::MessageManager::callAsync([this] { updateUI(); });
    };
    addAndMakeVisible(modeBox);
    
    // Color picker for sender
    colorPicker.setColor(proc.getTrackColor());
    colorPicker.onChanged = [this](float h, float s, float b) {
        proc.apvts.getParameter("hue")->setValueNotifyingHost(h);
        proc.apvts.getParameter("sat")->setValueNotifyingHost(s);
        proc.apvts.getParameter("bri")->setValueNotifyingHost(b);
    };
    addAndMakeVisible(colorPicker);
    
    // Status label
    statusLbl.setFont(juce::FontOptions(12.0f));
    statusLbl.setColour(juce::Label::textColourId, UI::textDim);
    addAndMakeVisible(statusLbl);
    
    // View mode selector (for receiver)
    viewBox.addItem("3D Perspective", 1);
    viewBox.addItem("Top (Freq vs Stereo)", 2);
    viewBox.addItem("Side (Freq vs Level)", 3);
    viewBox.setSelectedId(1, juce::dontSendNotification);
    viewBox.setColour(juce::ComboBox::backgroundColourId, UI::panel);
    viewBox.setColour(juce::ComboBox::textColourId, UI::text);
    viewBox.setColour(juce::ComboBox::outlineColourId, UI::border);
    viewBox.onChange = [this] {
        if (renderer == nullptr) return;
        int id = viewBox.getSelectedId();
        if (id == 1) renderer->setViewMode(ViewMode::Perspective3D);
        else if (id == 2) renderer->setViewMode(ViewMode::TopFlat);
        else renderer->setViewMode(ViewMode::SideFlat);
    };
    addChildComponent(viewBox);
    
    // Reset button
    resetBtn.setColour(juce::TextButton::buttonColourId, UI::panel);
    resetBtn.setColour(juce::TextButton::textColourOffId, UI::text);
    resetBtn.onClick = [this] { if (renderer != nullptr) renderer->resetView(); };
    addChildComponent(resetBtn);
    
    // Range slider
    rangeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    rangeSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    rangeSlider.setColour(juce::Slider::backgroundColourId, UI::bg1);
    rangeSlider.setColour(juce::Slider::trackColourId, UI::panel);
    rangeSlider.setColour(juce::Slider::thumbColourId, UI::text);
    rangeSlider.setColour(juce::Slider::textBoxTextColourId, UI::text);
    rangeSlider.setColour(juce::Slider::textBoxBackgroundColourId, UI::bg1);
    rangeSlider.setColour(juce::Slider::textBoxOutlineColourId, UI::border);
    rangeSlider.setTextValueSuffix(" dB");
    addChildComponent(rangeSlider);
    
    rangeLabel.setText("Range:", juce::dontSendNotification);
    rangeLabel.setFont(juce::FontOptions(12.0f));
    rangeLabel.setColour(juce::Label::textColourId, UI::textDim);
    addChildComponent(rangeLabel);
    
    rangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "range", rangeSlider);
    
    // High Res toggle (for sender mode - affects analysis)
    highResBtn.setColour(juce::ToggleButton::textColourId, UI::text);
    highResBtn.setColour(juce::ToggleButton::tickColourId, UI::text);
    highResBtn.setColour(juce::ToggleButton::tickDisabledColourId, UI::textDim);
    highResBtn.onClick = [this] {
        // Direct callback to ensure parameter change is processed
        bool highRes = highResBtn.getToggleState();
        int newBands = highRes ? 48 : 24;
        proc.setNumBands(newBands);
    };
    addAndMakeVisible(highResBtn);
    
    highResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "highres", highResBtn);
    
    // Create receiver components if needed
    if (proc.getMode() == PluginMode::Receiver)
    {
        renderer = std::make_unique<Spectral3DRenderer>(proc.getSharedData(), 
            proc.apvts.getRawParameterValue("range"));
        addChildComponent(*renderer);
        
        trackList = std::make_unique<TrackList>(proc.getSharedData());
        addChildComponent(*trackList);
    }
    
    uiInitialized = true;
    updateUI();
    startTimerHz(10);
}

SpectralImagerAudioProcessorEditor::~SpectralImagerAudioProcessorEditor()
{
    stopTimer();
}

void SpectralImagerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(UI::bg1);
    
    // Header bar
    g.setColour(UI::bg2);
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(UI::border);
    g.drawLine(0, 50, static_cast<float>(getWidth()), 50, 1.0f);
}

void SpectralImagerAudioProcessorEditor::resized()
{
    if (!uiInitialized) return;
    
    auto b = getLocalBounds();
    
    // Header
    auto header = b.removeFromTop(50);
    title.setBounds(header.removeFromLeft(180).reduced(10, 10));
    header.removeFromLeft(20);
    modeBox.setBounds(header.removeFromLeft(120).reduced(5, 12));
    
    b.reduce(10, 10);
    
    bool isSender = proc.getMode() == PluginMode::Sender;
    
    if (isSender)
    {
        // Sender layout
        auto panel = b.removeFromLeft(280);
        panel.removeFromTop(10);
        
        colorPicker.setBounds(panel.removeFromTop(80));
        panel.removeFromTop(15);
        highResBtn.setBounds(panel.removeFromTop(24));
        panel.removeFromTop(15);
        statusLbl.setBounds(panel.removeFromTop(60));
    }
    else
    {
        // Receiver layout - only if components exist
        if (renderer != nullptr && trackList != nullptr)
        {
            auto bottom = b.removeFromBottom(36);
            
            trackList->setBounds(bottom.removeFromLeft(180));
            bottom.removeFromLeft(10);
            
            rangeLabel.setBounds(bottom.removeFromLeft(45));
            rangeSlider.setBounds(bottom.removeFromLeft(120));
            bottom.removeFromLeft(10);
            
            viewBox.setBounds(bottom.removeFromLeft(140));
            bottom.removeFromLeft(10);
            resetBtn.setBounds(bottom.removeFromLeft(70));
            
            b.removeFromBottom(5);
            renderer->setBounds(b);
        }
    }
}

void SpectralImagerAudioProcessorEditor::timerCallback()
{
    // Update status for sender mode
    if (proc.getMode() == PluginMode::Sender)
    {
        int s = proc.getSlot();
        juce::String status = s >= 0
            ? "Status: Active on slot " + juce::String(s + 1)
            : "Status: No slot available";
        if (statusLbl.getText() != status)
            statusLbl.setText(status, juce::dontSendNotification);
    }
    
    // Sync mode selector with processor state
    bool isSender = proc.getMode() == PluginMode::Sender;
    int expectedId = isSender ? 1 : 2;
    if (modeBox.getSelectedId() != expectedId)
    {
        modeBox.setSelectedId(expectedId, juce::dontSendNotification);
        updateUI();
    }
}

void SpectralImagerAudioProcessorEditor::updateUI()
{
    if (!uiInitialized) return;
    
    bool isSender = proc.getMode() == PluginMode::Sender;
    
    // Show/hide sender UI
    colorPicker.setVisible(isSender);
    statusLbl.setVisible(isSender);
    
    // Create or destroy receiver components as needed
    if (!isSender)
    {
        // Create receiver components if they don't exist
        if (renderer == nullptr)
        {
            renderer = std::make_unique<Spectral3DRenderer>(proc.getSharedData(),
                proc.apvts.getRawParameterValue("range"));
            addAndMakeVisible(*renderer);
        }
        if (trackList == nullptr)
        {
            trackList = std::make_unique<TrackList>(proc.getSharedData());
            addAndMakeVisible(*trackList);
        }
        
        renderer->setVisible(true);
        trackList->setVisible(true);
        viewBox.setVisible(true);
        resetBtn.setVisible(true);
        rangeSlider.setVisible(true);
        rangeLabel.setVisible(true);
        highResBtn.setVisible(false);  // Hide in receiver mode
    }
    else
    {
        // Hide receiver components (but don't destroy - might switch back)
        if (renderer != nullptr) renderer->setVisible(false);
        if (trackList != nullptr) trackList->setVisible(false);
        viewBox.setVisible(false);
        resetBtn.setVisible(false);
        rangeSlider.setVisible(false);
        rangeLabel.setVisible(false);
        highResBtn.setVisible(true);  // Show in sender mode
    }
    
    resized();
    repaint();
}
