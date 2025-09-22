#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorDistortionAudioProcessorEditor::WarriorDistortionAudioProcessorEditor (WarriorDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Set up title
    titleLabel.setText("WARRIOR DISTORTION", juce::dontSendNotification);
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
    
    setupSlider(inputGainSlider, inputGainLabel, "Input");
    setupSlider(outputGainSlider, outputGainLabel, "Output");
    setupSlider(asymmetrySlider, asymmetryLabel, "Asymmetry");
    
    // Set up distortion stages
    for (int i = 0; i < 3; ++i)
    {
        juce::String stageName = "Stage " + juce::String(i + 1);
        
        stageEnableButtons[i].setButtonText("Enable");
        addAndMakeVisible(stageEnableButtons[i]);
        
        stageTypeCombo[i].addItem("Overdrive", 1);
        stageTypeCombo[i].addItem("Fuzz", 2);
        stageTypeCombo[i].addItem("Tube", 3);
        stageTypeCombo[i].addItem("Bitcrush", 4);
        stageTypeCombo[i].addItem("Waveshaper", 5);
        addAndMakeVisible(stageTypeCombo[i]);
        
        stageDriveSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        stageDriveSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 16);
        addAndMakeVisible(stageDriveSliders[i]);
        
        stageGainSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        stageGainSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 16);
        addAndMakeVisible(stageGainSliders[i]);
        
        stageLabels[i].setText(stageName, juce::dontSendNotification);
        stageLabels[i].setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(stageLabels[i]);
    }
    
    // Set up tone stack
    setupSlider(bassSlider, bassLabel, "Bass");
    setupSlider(midSlider, midLabel, "Mid");
    setupSlider(trebleSlider, trebleLabel, "Treble");
    
    // Set up cabinet simulation
    cabEnableButton.setButtonText("Cabinet");
    addAndMakeVisible(cabEnableButton);
    
    setupSlider(cabCutoffSlider, cabCutoffLabel, "Cutoff");
    
    // Create parameter attachments
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "inputGain", inputGainSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "outputGain", outputGainSlider);
    asymmetryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "asymmetry", asymmetrySlider);
    
    for (int i = 0; i < 3; ++i)
    {
        juce::String stagePrefix = "stage" + juce::String(i + 1);
        
        stageEnableAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
            (audioProcessor.parameters, stagePrefix + "Enable", stageEnableButtons[i]);
        stageTypeAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
            (audioProcessor.parameters, stagePrefix + "Type", stageTypeCombo[i]);
        stageDriveAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.parameters, stagePrefix + "Drive", stageDriveSliders[i]);
        stageGainAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
            (audioProcessor.parameters, stagePrefix + "Gain", stageGainSliders[i]);
    }
    
    bassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "bass", bassSlider);
    midAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "mid", midSlider);
    trebleAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "treble", trebleSlider);
    
    cabEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>
        (audioProcessor.parameters, "cabEnable", cabEnableButton);
    cabCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>
        (audioProcessor.parameters, "cabCutoff", cabCutoffSlider);

    setSize(800, 600);
}

WarriorDistortionAudioProcessorEditor::~WarriorDistortionAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void WarriorDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(WarriorAudio::WarriorLookAndFeel::Colors::background);
    
    // Draw main frame
    customLookAndFeel.drawWarriorFrame(g, getLocalBounds().reduced(10));
    
    auto bounds = getLocalBounds().reduced(20);
    
    // Input section
    auto inputBounds = bounds.removeFromTop(100);
    customLookAndFeel.drawWarriorFrame(g, inputBounds, "Input / Output");
    
    // Distortion stages section
    auto stagesBounds = bounds.removeFromTop(180);
    customLookAndFeel.drawWarriorFrame(g, stagesBounds, "Distortion Stages");
    
    // Draw column headers for stages
    auto headerBounds = stagesBounds.removeFromTop(60).reduced(10);
    headerBounds.removeFromTop(30); // Skip section title space
    
    int stageWidth = headerBounds.getWidth() / 5;
    headerBounds.removeFromLeft(stageWidth); // Skip first column (stage names)
    
    g.setColour(WarriorAudio::WarriorLookAndFeel::Colors::textSecondary);
    g.setFont(12.0f);
    g.drawText("Type", headerBounds.removeFromLeft(stageWidth), juce::Justification::centred);
    g.drawText("Drive", headerBounds.removeFromLeft(stageWidth), juce::Justification::centred);
    g.drawText("Gain", headerBounds.removeFromLeft(stageWidth), juce::Justification::centred);
    g.drawText("Enable", headerBounds.removeFromLeft(stageWidth), juce::Justification::centred);
    
    // Tone stack section
    auto toneBounds = bounds.removeFromTop(120);
    customLookAndFeel.drawWarriorFrame(g, toneBounds, "Tone Stack");
    
    // Cabinet section
    auto cabBounds = bounds;
    customLookAndFeel.drawWarriorFrame(g, cabBounds, "Cabinet Simulation");
}

void WarriorDistortionAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    
    // Input section
    auto inputBounds = bounds.removeFromTop(100).reduced(10);
    inputBounds.removeFromTop(30); // Space for section title
    
    int inputSliderWidth = inputBounds.getWidth() / 3 - 10;
    inputGainSlider.setBounds(inputBounds.removeFromLeft(inputSliderWidth));
    inputBounds.removeFromLeft(15);
    outputGainSlider.setBounds(inputBounds.removeFromLeft(inputSliderWidth));
    inputBounds.removeFromLeft(15);
    asymmetrySlider.setBounds(inputBounds.removeFromLeft(inputSliderWidth));
    
    bounds.removeFromTop(10);
    
    // Distortion stages section
    auto stagesBounds = bounds.removeFromTop(180).reduced(10);
    stagesBounds.removeFromTop(60); // Space for section title and column headers
    
    int stageHeight = 35;
    int stageWidth = stagesBounds.getWidth() / 5;
    
    for (int i = 0; i < 3; ++i)
    {
        auto stageRowBounds = stagesBounds.removeFromTop(stageHeight);
        
        stageLabels[i].setBounds(stageRowBounds.removeFromLeft(stageWidth));
        stageTypeCombo[i].setBounds(stageRowBounds.removeFromLeft(stageWidth).reduced(5));
        stageDriveSliders[i].setBounds(stageRowBounds.removeFromLeft(stageWidth).reduced(5));
        stageGainSliders[i].setBounds(stageRowBounds.removeFromLeft(stageWidth).reduced(5));
        stageEnableButtons[i].setBounds(stageRowBounds.removeFromLeft(stageWidth).reduced(5, 5));
        
        stagesBounds.removeFromTop(5); // Space between rows
    }
    
    bounds.removeFromTop(10);
    
    // Tone stack section
    auto toneBounds = bounds.removeFromTop(120).reduced(10);
    toneBounds.removeFromTop(30); // Space for section title
    
    int toneSliderWidth = toneBounds.getWidth() / 3 - 10;
    bassSlider.setBounds(toneBounds.removeFromLeft(toneSliderWidth));
    toneBounds.removeFromLeft(15);
    midSlider.setBounds(toneBounds.removeFromLeft(toneSliderWidth));
    toneBounds.removeFromLeft(15);
    trebleSlider.setBounds(toneBounds.removeFromLeft(toneSliderWidth));
    
    bounds.removeFromTop(10);
    
    // Cabinet section
    auto cabBounds = bounds.reduced(10);
    cabBounds.removeFromTop(30); // Space for section title
    
    int cabSliderWidth = cabBounds.getWidth() / 2 - 10;
    cabEnableButton.setBounds(cabBounds.removeFromTop(30).removeFromLeft(100));
    cabCutoffSlider.setBounds(cabBounds.removeFromLeft(cabSliderWidth));
}