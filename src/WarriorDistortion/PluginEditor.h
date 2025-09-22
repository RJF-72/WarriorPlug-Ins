#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../common/WarriorLookAndFeel.h"

class WarriorDistortionAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    WarriorDistortionAudioProcessorEditor (WarriorDistortionAudioProcessor&);
    ~WarriorDistortionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WarriorDistortionAudioProcessor& audioProcessor;
    WarriorAudio::WarriorLookAndFeel customLookAndFeel;

    // Main controls
    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider asymmetrySlider;
    
    juce::Label inputGainLabel;
    juce::Label outputGainLabel;
    juce::Label asymmetryLabel;
    juce::Label titleLabel;

    // Distortion stages
    std::array<juce::ToggleButton, 3> stageEnableButtons;
    std::array<juce::ComboBox, 3> stageTypeCombo;
    std::array<juce::Slider, 3> stageDriveSliders;
    std::array<juce::Slider, 3> stageGainSliders;
    
    std::array<juce::Label, 3> stageLabels;

    // Tone stack
    juce::Slider bassSlider;
    juce::Slider midSlider;
    juce::Slider trebleSlider;
    
    juce::Label bassLabel;
    juce::Label midLabel;
    juce::Label trebleLabel;

    // Cabinet simulation
    juce::ToggleButton cabEnableButton;
    juce::Slider cabCutoffSlider;
    
    juce::Label cabCutoffLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> asymmetryAttachment;
    
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 3> stageEnableAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 3> stageTypeAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> stageDriveAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 3> stageGainAttachments;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> trebleAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> cabEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cabCutoffAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarriorDistortionAudioProcessorEditor)
};