#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorDelayAudioProcessorEditor::WarriorDelayAudioProcessorEditor (WarriorDelayAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Set up title
    titleLabel.setText("WARRIOR DELAY", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, WarriorAudio::WarriorLookAndFeel::Colors::primary);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up master controls
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& labelText)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
        addAndMakeVisible(slider);
        
        label.setText(labelText, juce::dontSendNotification);
        label.attachToComponent(&slider, false);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };
    
    setupSlider(masterFeedbackSlider, masterFeedbackLabel, "Master Feedback");
    setupSlider(wetLevelSlider, wetLevelLabel, "Wet");
    setupSlider(dryLevelSlider, dryLevelLabel, "Dry");
    
    tempoSyncButton.setButtonText("Tempo Sync");
    addAndMakeVisible(tempoSyncButton);
    
    // Set up LFO controls
    setupSlider(lfo1RateSlider, lfo1RateLabel, "LFO 1 Rate");
    setupSlider(lfo1DepthSlider, lfo1DepthLabel, "LFO 1 Depth");
    setupSlider(lfo2RateSlider, lfo2RateLabel, "LFO 2 Rate");
    setupSlider(lfo2DepthSlider, lfo2DepthLabel, "LFO 2 Depth");
    
    lfo1ShapeCombo.addItem("Sine", 1);
    lfo1ShapeCombo.addItem("Triangle", 2);
    lfo1ShapeCombo.addItem("Square", 3);
    lfo1ShapeCombo.addItem("Saw", 4);
    addAndMakeVisible(lfo1ShapeCombo);
    
    lfo1ShapeLabel.setText("LFO 1 Shape", juce::dontSendNotification);
    lfo1ShapeLabel.attachToComponent(&lfo1ShapeCombo, false);
    addAndMakeVisible(lfo1ShapeLabel);
    
    lfo2ShapeCombo.addItem("Sine", 1);
    lfo2ShapeCombo.addItem("Triangle", 2);
    lfo2ShapeCombo.addItem("Square", 3);
    lfo2ShapeCombo.addItem("Saw", 4);
    addAndMakeVisible(lfo2ShapeCombo);
    
    lfo2ShapeLabel.setText("LFO 2 Shape", juce::dontSendNotification);
    lfo2ShapeLabel.attachToComponent(&lfo2ShapeCombo, false);
    addAndMakeVisible(lfo2ShapeLabel);
    
    // Set up delay tap controls
    for (int i = 0; i < 4; ++i)
    {
        juce::String tapName = "Tap " + juce::String(i + 1);
        
        tapEnableButtons[i].setButtonText("Enable");
        addAndMakeVisible(tapEnableButtons[i]);
        
        tapTimeSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        tapTimeSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 16);
        addAndMakeVisible(tapTimeSliders[i]);
        
        tapLevelSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        tapLevelSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 16);
        addAndMakeVisible(tapLevelSliders[i]);
        
        tapFeedbackSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        tapFeedbackSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 16);
        addAndMakeVisible(tapFeedbackSliders[i]);
        
        tapLabels[i].setText(tapName, juce::dontSendNotification);
        tapLabels[i].setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(tapLabels[i]);
    }
    
    // Create parameter attachments
    masterFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "masterFeedback", masterFeedbackSlider);
    wetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "wetLevel", wetLevelSlider);
    dryLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "dryLevel", dryLevelSlider);
    tempoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "tempoSync", tempoSyncButton);
    
    lfo1RateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "lfo1Rate", lfo1RateSlider);
    lfo1DepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "lfo1Depth", lfo1DepthSlider);
    lfo1ShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.parameters, "lfo1Shape", lfo1ShapeCombo);
    lfo2RateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "lfo2Rate", lfo2RateSlider);
    lfo2DepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "lfo2Depth", lfo2DepthSlider);
    lfo2ShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.parameters, "lfo2Shape", lfo2ShapeCombo);
    
    for (int i = 0; i < 4; ++i)
    {
        juce::String paramPrefix = "tap" + juce::String(i + 1);
        
        tapEnableAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
            (audioProcessor.parameters, paramPrefix + "Enable", tapEnableButtons[i]);
        tapTimeAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.parameters, paramPrefix + "Time", tapTimeSliders[i]);
        tapLevelAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.parameters, paramPrefix + "Level", tapLevelSliders[i]);
        tapFeedbackAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.parameters, paramPrefix + "Feedback", tapFeedbackSliders[i]);
    }

    setSize(800, 600);
}

WarriorDelayAudioProcessorEditor::~WarriorDelayAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void WarriorDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(WarriorAudio::WarriorLookAndFeel::Colors::background);
    
    // Draw main frame
    customLookAndFeel.drawWarriorFrame(g, getLocalBounds().reduced(10));
    
    auto bounds = getLocalBounds().reduced(20);
    
    // Master section
    auto masterBounds = bounds.removeFromTop(120);
    customLookAndFeel.drawWarriorFrame(g, masterBounds, "Master Controls");
    
    // LFO section
    auto lfoBounds = bounds.removeFromTop(120);
    customLookAndFeel.drawWarriorFrame(g, lfoBounds, "Modulation");
    
    // Delay taps section
    auto tapsBounds = bounds;
    customLookAndFeel.drawWarriorFrame(g, tapsBounds, "Delay Taps");
    
    // Draw labels for delay tap columns
    auto headerBounds = tapsBounds.removeFromTop(60).reduced(10);
    headerBounds.removeFromTop(30); // Skip section title space
    
    int tapWidth = headerBounds.getWidth() / 5;
    headerBounds.removeFromLeft(tapWidth); // Skip first column (tap names)
    
    g.setColour(WarriorAudio::WarriorLookAndFeel::Colors::textSecondary);
    g.setFont(12.0f);
    g.drawText("Time", headerBounds.removeFromLeft(tapWidth), juce::Justification::centred);
    g.drawText("Level", headerBounds.removeFromLeft(tapWidth), juce::Justification::centred);
    g.drawText("Feedback", headerBounds.removeFromLeft(tapWidth), juce::Justification::centred);
    g.drawText("Enable", headerBounds.removeFromLeft(tapWidth), juce::Justification::centred);
}

void WarriorDelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    // Master section
    auto masterBounds = bounds.removeFromTop(120).reduced(10);
    masterBounds.removeFromTop(30); // Space for section title
    
    int masterSliderWidth = masterBounds.getWidth() / 4 - 10;
    masterFeedbackSlider.setBounds(masterBounds.removeFromLeft(masterSliderWidth));
    masterBounds.removeFromLeft(10);
    wetLevelSlider.setBounds(masterBounds.removeFromLeft(masterSliderWidth));
    masterBounds.removeFromLeft(10);
    dryLevelSlider.setBounds(masterBounds.removeFromLeft(masterSliderWidth));
    masterBounds.removeFromLeft(10);
    tempoSyncButton.setBounds(masterBounds.removeFromTop(30));
    
    bounds.removeFromTop(10);
    
    // LFO section
    auto lfoBounds = bounds.removeFromTop(120).reduced(10);
    lfoBounds.removeFromTop(30); // Space for section title
    
    auto lfo1Bounds = lfoBounds.removeFromLeft(lfoBounds.getWidth() / 2);
    auto lfo2Bounds = lfoBounds;
    
    int lfoSliderWidth = lfo1Bounds.getWidth() / 3 - 5;
    lfo1RateSlider.setBounds(lfo1Bounds.removeFromLeft(lfoSliderWidth));
    lfo1Bounds.removeFromLeft(5);
    lfo1DepthSlider.setBounds(lfo1Bounds.removeFromLeft(lfoSliderWidth));
    lfo1Bounds.removeFromLeft(5);
    lfo1ShapeCombo.setBounds(lfo1Bounds.removeFromTop(25));
    
    lfo2RateSlider.setBounds(lfo2Bounds.removeFromLeft(lfoSliderWidth));
    lfo2Bounds.removeFromLeft(5);
    lfo2DepthSlider.setBounds(lfo2Bounds.removeFromLeft(lfoSliderWidth));
    lfo2Bounds.removeFromLeft(5);
    lfo2ShapeCombo.setBounds(lfo2Bounds.removeFromTop(25));
    
    bounds.removeFromTop(10);
    
    // Delay taps section
    auto tapsBounds = bounds.reduced(10);
    tapsBounds.removeFromTop(60); // Space for section title and column headers
    
    int tapHeight = 40;
    int tapWidth = tapsBounds.getWidth() / 5;
    
    for (int i = 0; i < 4; ++i)
    {
        auto tapRowBounds = tapsBounds.removeFromTop(tapHeight);
        
        tapLabels[i].setBounds(tapRowBounds.removeFromLeft(tapWidth));
        tapTimeSliders[i].setBounds(tapRowBounds.removeFromLeft(tapWidth).reduced(5));
        tapLevelSliders[i].setBounds(tapRowBounds.removeFromLeft(tapWidth).reduced(5));
        tapFeedbackSliders[i].setBounds(tapRowBounds.removeFromLeft(tapWidth).reduced(5));
        tapEnableButtons[i].setBounds(tapRowBounds.removeFromLeft(tapWidth).reduced(5, 8));
        
        tapsBounds.removeFromTop(5); // Space between rows
    }
}