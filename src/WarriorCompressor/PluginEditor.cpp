#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorCompressorAudioProcessorEditor::WarriorCompressorAudioProcessorEditor (WarriorCompressorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Set up title
    titleLabel.setText("WARRIOR COMPRESSOR", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, WarriorAudio::WarriorLookAndFeel::Colors::primary);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up main controls
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
    
    setupSlider(thresholdSlider, thresholdLabel, "Threshold");
    setupSlider(ratioSlider, ratioLabel, "Ratio");
    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(releaseSlider, releaseLabel, "Release");
    setupSlider(kneeSlider, kneeLabel, "Knee");
    setupSlider(makeupGainSlider, makeupGainLabel, "Makeup");
    setupSlider(wetLevelSlider, wetLevelLabel, "Mix");
    setupSlider(lookAheadSlider, lookAheadLabel, "Look Ahead");
    
    // Set up advanced controls
    autoMakeupButton.setButtonText("Auto Makeup");
    addAndMakeVisible(autoMakeupButton);
    
    sidechainEnableButton.setButtonText("Sidechain");
    addAndMakeVisible(sidechainEnableButton);
    
    setupSlider(sidechainHPFSlider, sidechainHPFLabel, "SC HPF");
    
    // Create parameter attachments
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "ratio", ratioSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "release", releaseSlider);
    kneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "knee", kneeSlider);
    makeupGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "makeupGain", makeupGainSlider);
    wetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "wetLevel", wetLevelSlider);
    lookAheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "lookAhead", lookAheadSlider);
    
    autoMakeupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "autoMakeup", autoMakeupButton);
    sidechainEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "sidechainEnable", sidechainEnableButton);
    sidechainHPFAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "sidechainHPF", sidechainHPFSlider);

    // Start timer for level metering
    startTimerHz(30); // 30 FPS update rate

    setSize(700, 500);
}

WarriorCompressorAudioProcessorEditor::~WarriorCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void WarriorCompressorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(WarriorAudio::WarriorLookAndFeel::Colors::background);
    
    // Draw main frame
    customLookAndFeel.drawWarriorFrame(g, getLocalBounds().reduced(10));
    
    auto bounds = getLocalBounds().reduced(20);
    
    // Main controls section
    auto mainBounds = bounds.removeFromTop(180);
    customLookAndFeel.drawWarriorFrame(g, mainBounds, "Compressor");
    
    // Advanced controls section
    auto advancedBounds = bounds.removeFromTop(120);
    customLookAndFeel.drawWarriorFrame(g, advancedBounds, "Advanced");
    
    // Meters section
    auto metersBounds = bounds;
    customLookAndFeel.drawWarriorFrame(g, metersBounds, "Meters");
    
    // Draw meter labels
    auto meterLabelBounds = metersBounds.reduced(10);
    meterLabelBounds.removeFromTop(30); // Skip section title
    
    g.setColour(WarriorAudio::WarriorLookAndFeel::Colors::textSecondary);
    g.setFont(12.0f);
    
    int meterWidth = meterLabelBounds.getWidth() / 3;
    g.drawText("Input", meterLabelBounds.removeFromLeft(meterWidth), juce::Justification::centred);
    g.drawText("GR", meterLabelBounds.removeFromLeft(meterWidth), juce::Justification::centred);
    g.drawText("Output", meterLabelBounds.removeFromLeft(meterWidth), juce::Justification::centred);
    
    // Draw level meters
    customLookAndFeel.drawMeterBar(g, inputMeterBounds, inputLevel, false);
    customLookAndFeel.drawMeterBar(g, grMeterBounds, gainReduction, false);
    customLookAndFeel.drawMeterBar(g, outputMeterBounds, outputLevel, false);
}

void WarriorCompressorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    // Main controls section
    auto mainBounds = bounds.removeFromTop(180).reduced(10);
    mainBounds.removeFromTop(30); // Space for section title
    
    // Top row - main compressor controls
    auto topRow = mainBounds.removeFromTop(90);
    int mainSliderWidth = topRow.getWidth() / 4 - 10;
    
    thresholdSlider.setBounds(topRow.removeFromLeft(mainSliderWidth));
    topRow.removeFromLeft(13);
    ratioSlider.setBounds(topRow.removeFromLeft(mainSliderWidth));
    topRow.removeFromLeft(13);
    attackSlider.setBounds(topRow.removeFromLeft(mainSliderWidth));
    topRow.removeFromLeft(13);
    releaseSlider.setBounds(topRow.removeFromLeft(mainSliderWidth));
    
    // Bottom row - additional controls
    auto bottomRow = mainBounds;
    kneeSlider.setBounds(bottomRow.removeFromLeft(mainSliderWidth));
    bottomRow.removeFromLeft(13);
    makeupGainSlider.setBounds(bottomRow.removeFromLeft(mainSliderWidth));
    bottomRow.removeFromLeft(13);
    wetLevelSlider.setBounds(bottomRow.removeFromLeft(mainSliderWidth));
    bottomRow.removeFromLeft(13);
    lookAheadSlider.setBounds(bottomRow.removeFromLeft(mainSliderWidth));
    
    bounds.removeFromTop(10);
    
    // Advanced controls section
    auto advancedBounds = bounds.removeFromTop(120).reduced(10);
    advancedBounds.removeFromTop(30); // Space for section title
    
    int advancedSliderWidth = advancedBounds.getWidth() / 3 - 10;
    
    auto buttonArea = advancedBounds.removeFromLeft(advancedSliderWidth);
    autoMakeupButton.setBounds(buttonArea.removeFromTop(30));
    buttonArea.removeFromTop(5);
    sidechainEnableButton.setBounds(buttonArea.removeFromTop(30));
    
    advancedBounds.removeFromLeft(15);
    sidechainHPFSlider.setBounds(advancedBounds.removeFromLeft(advancedSliderWidth));
    
    bounds.removeFromTop(10);
    
    // Meters section
    auto metersBounds = bounds.reduced(10);
    metersBounds.removeFromTop(50); // Space for section title and labels
    
    int meterWidth = metersBounds.getWidth() / 3 - 10;
    int meterHeight = metersBounds.getHeight();
    
    inputMeterBounds = metersBounds.removeFromLeft(meterWidth).withHeight(meterHeight);
    metersBounds.removeFromLeft(15);
    grMeterBounds = metersBounds.removeFromLeft(meterWidth).withHeight(meterHeight);
    metersBounds.removeFromLeft(15);
    outputMeterBounds = metersBounds.removeFromLeft(meterWidth).withHeight(meterHeight);
}

void WarriorCompressorAudioProcessorEditor::timerCallback()
{
    // Update meter levels
    inputLevel = audioProcessor.getCurrentInputLevel();
    outputLevel = audioProcessor.getCurrentOutputLevel();
    gainReduction = std::abs(audioProcessor.getCurrentGainReduction()) / 60.0f; // Normalize to 0-1 range
    
    // Repaint meters area
    auto meterArea = juce::Rectangle<int>();
    meterArea = meterArea.getUnion(inputMeterBounds);
    meterArea = meterArea.getUnion(grMeterBounds);
    meterArea = meterArea.getUnion(outputMeterBounds);
    repaint(meterArea);
}