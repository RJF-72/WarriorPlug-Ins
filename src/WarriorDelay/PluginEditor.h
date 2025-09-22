#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../common/WarriorLookAndFeel.h"

class WarriorDelayAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    WarriorDelayAudioProcessorEditor (WarriorDelayAudioProcessor&);
    ~WarriorDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WarriorDelayAudioProcessor& audioProcessor;
    WarriorAudio::WarriorLookAndFeel customLookAndFeel;

    // Master controls
    juce::Slider masterFeedbackSlider;
    juce::Slider wetLevelSlider;
    juce::Slider dryLevelSlider;
    juce::ToggleButton tempoSyncButton;
    
    juce::Label masterFeedbackLabel;
    juce::Label wetLevelLabel;
    juce::Label dryLevelLabel;
    juce::Label titleLabel;

    // LFO controls
    juce::Slider lfo1RateSlider;
    juce::Slider lfo1DepthSlider;
    juce::ComboBox lfo1ShapeCombo;
    juce::Slider lfo2RateSlider;
    juce::Slider lfo2DepthSlider;
    juce::ComboBox lfo2ShapeCombo;
    
    juce::Label lfo1RateLabel;
    juce::Label lfo1DepthLabel;
    juce::Label lfo1ShapeLabel;
    juce::Label lfo2RateLabel;
    juce::Label lfo2DepthLabel;
    juce::Label lfo2ShapeLabel;

    // Delay tap controls (simplified for UI)
    std::array<juce::ToggleButton, 4> tapEnableButtons;
    std::array<juce::Slider, 4> tapTimeSliders;
    std::array<juce::Slider, 4> tapLevelSliders;
    std::array<juce::Slider, 4> tapFeedbackSliders;
    
    std::array<juce::Label, 4> tapLabels;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterFeedbackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> tempoSyncAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1DepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2DepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo2ShapeAttachment;
    
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 4> tapEnableAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> tapTimeAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> tapLevelAttachments;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> tapFeedbackAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarriorDelayAudioProcessorEditor)
};