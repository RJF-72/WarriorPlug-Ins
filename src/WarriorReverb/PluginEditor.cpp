#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorReverbAudioProcessorEditor::WarriorReverbAudioProcessorEditor (WarriorReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Set up title
    titleLabel.setText("WARRIOR REVERB", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, WarriorAudio::WarriorLookAndFeel::Colors::primary);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Set up reverb type combo box
    reverbTypeCombo.addItem("Hall", 1);
    reverbTypeCombo.addItem("Room", 2);
    reverbTypeCombo.addItem("Plate", 3);
    reverbTypeCombo.addItem("Spring", 4);
    reverbTypeCombo.addItem("Shimmer", 5);
    addAndMakeVisible(reverbTypeCombo);
    
    reverbTypeLabel.setText("Type", juce::dontSendNotification);
    reverbTypeLabel.attachToComponent(&reverbTypeCombo, false);
    addAndMakeVisible(reverbTypeLabel);
    
    // Set up sliders
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& labelText)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(slider);
        
        label.setText(labelText, juce::dontSendNotification);
        label.attachToComponent(&slider, false);
        addAndMakeVisible(label);
    };
    
    setupSlider(roomSizeSlider, roomSizeLabel, "Room Size");
    setupSlider(dampingSlider, dampingLabel, "Damping");
    setupSlider(diffusionSlider, diffusionLabel, "Diffusion");
    setupSlider(wetLevelSlider, wetLevelLabel, "Wet");
    setupSlider(dryLevelSlider, dryLevelLabel, "Dry");
    
    // Create parameter attachments
    reverbTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        (audioProcessor.parameters, "reverbType", reverbTypeCombo);
    roomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "roomSize", roomSizeSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "damping", dampingSlider);
    diffusionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "diffusion", diffusionSlider);
    wetLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "wetLevel", wetLevelSlider);
    dryLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "dryLevel", dryLevelSlider);

    setSize(600, 400);
}

WarriorReverbAudioProcessorEditor::~WarriorReverbAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void WarriorReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(WarriorAudio::WarriorLookAndFeel::Colors::background);
    
    // Draw main frame
    customLookAndFeel.drawWarriorFrame(g, getLocalBounds().reduced(10));
    
    // Draw sections
    auto bounds = getLocalBounds().reduced(20);
    
    // Algorithm section
    auto algorithmBounds = bounds.removeFromTop(80);
    customLookAndFeel.drawWarriorFrame(g, algorithmBounds, "Algorithm");
    
    // Parameters section
    auto paramsBounds = bounds.removeFromTop(200);
    customLookAndFeel.drawWarriorFrame(g, paramsBounds, "Parameters");
    
    // Mix section
    auto mixBounds = bounds;
    customLookAndFeel.drawWarriorFrame(g, mixBounds, "Mix");
}

void WarriorReverbAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    // Algorithm section
    auto algorithmBounds = bounds.removeFromTop(80).reduced(10);
    algorithmBounds.removeFromTop(25); // Space for section title
    reverbTypeCombo.setBounds(algorithmBounds.withHeight(30));
    bounds.removeFromTop(10);
    
    // Parameters section
    auto paramsBounds = bounds.removeFromTop(200).reduced(10);
    paramsBounds.removeFromTop(25); // Space for section title
    
    int sliderWidth = paramsBounds.getWidth() / 3 - 10;
    int sliderHeight = 120;
    
    auto topRow = paramsBounds.removeFromTop(sliderHeight);
    roomSizeSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    topRow.removeFromLeft(15);
    dampingSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    topRow.removeFromLeft(15);
    diffusionSlider.setBounds(topRow.removeFromLeft(sliderWidth));
    
    bounds.removeFromTop(10);
    
    // Mix section
    auto mixBounds = bounds.reduced(10);
    mixBounds.removeFromTop(25); // Space for section title
    
    int mixSliderWidth = mixBounds.getWidth() / 2 - 10;
    wetLevelSlider.setBounds(mixBounds.removeFromLeft(mixSliderWidth));
    mixBounds.removeFromLeft(20);
    dryLevelSlider.setBounds(mixBounds.removeFromLeft(mixSliderWidth));
}