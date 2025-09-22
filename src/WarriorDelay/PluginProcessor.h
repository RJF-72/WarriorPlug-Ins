#pragma once
#include <JuceHeader.h>
#include "../common/DSPUtils.h"

class WarriorDelayAudioProcessor : public juce::AudioProcessor
{
public:
    WarriorDelayAudioProcessor();
    ~WarriorDelayAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState parameters;

private:
    // Multi-tap delay system
    struct DelayTap
    {
        WarriorDSP::DelayLine delayLine;
        WarriorDSP::BiquadFilter filter;
        float delayTime = 0.25f; // seconds
        float feedback = 0.3f;
        float level = 0.5f;
        float pan = 0.0f; // -1 to 1
        bool enabled = true;
    };

    // Modulation system
    struct ModulationOscillator
    {
        float phase = 0.0f;
        float frequency = 1.0f; // Hz
        float depth = 0.0f;
        int waveform = 0; // 0=sine, 1=triangle, 2=square, 3=saw
        
        void updatePhase(float sampleRate);
        float getValue();
    };

    std::array<DelayTap, 4> delayTaps; // 4 independent delay taps
    std::array<ModulationOscillator, 2> lfoOscillators; // 2 LFOs for modulation
    
    double currentSampleRate = 44100.0;
    float syncedDelayTimes[8] = {1.0f, 0.5f, 0.25f, 0.125f, 0.75f, 0.375f, 0.1875f, 1.5f}; // Note divisions
    
    // Parameter helpers
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarriorDelayAudioProcessor)
};