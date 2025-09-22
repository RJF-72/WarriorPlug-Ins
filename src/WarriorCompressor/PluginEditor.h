#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../common/WarriorLookAndFeel.h"

class WarriorCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                              public juce::Timer
{
public:
    WarriorCompressorAudioProcessorEditor (WarriorCompressorAudioProcessor&);
    ~WarriorCompressorAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    WarriorCompressorAudioProcessor& audioProcessor;
    WarriorAudio::WarriorLookAndFeel customLookAndFeel;

    // Main controls
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider kneeSlider;
    juce::Slider makeupGainSlider;
    juce::Slider wetLevelSlider;
    juce::Slider lookAheadSlider;
    
    juce::Label thresholdLabel;
    juce::Label ratioLabel;
    juce::Label attackLabel;
    juce::Label releaseLabel;
    juce::Label kneeLabel;
    juce::Label makeupGainLabel;
    juce::Label wetLevelLabel;
    juce::Label lookAheadLabel;
    juce::Label titleLabel;

    // Advanced controls
    juce::ToggleButton autoMakeupButton;
    juce::ToggleButton sidechainEnableButton;
    juce::Slider sidechainHPFSlider;
    juce::Label sidechainHPFLabel;

    // Level meters
    juce::Rectangle<int> inputMeterBounds;
    juce::Rectangle<int> outputMeterBounds;
    juce::Rectangle<int> grMeterBounds;
    
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    float gainReduction = 0.0f;

    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lookAheadAttachment;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoMakeupAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sidechainEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sidechainHPFAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarriorCompressorAudioProcessorEditor)
};