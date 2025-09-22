#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../common/WarriorLookAndFeel.h"

class WarriorReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    WarriorReverbAudioProcessorEditor (WarriorReverbAudioProcessor&);
    ~WarriorReverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WarriorReverbAudioProcessor& audioProcessor;
    WarriorAudio::WarriorLookAndFeel customLookAndFeel;

    // UI Components
    juce::ComboBox reverbTypeCombo;
    juce::Slider roomSizeSlider;
    juce::Slider dampingSlider;
    juce::Slider diffusionSlider;
    juce::Slider wetLevelSlider;
    juce::Slider dryLevelSlider;
    
    juce::Label reverbTypeLabel;
    juce::Label roomSizeLabel;
    juce::Label dampingLabel;
    juce::Label diffusionLabel;
    juce::Label wetLevelLabel;
    juce::Label dryLevelLabel;
    juce::Label titleLabel;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> reverbTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> roomSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> diffusionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryLevelAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarriorReverbAudioProcessorEditor)
};