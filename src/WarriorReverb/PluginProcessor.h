#pragma once
#include <JuceHeader.h>
#include "../common/DSPUtils.h"

class WarriorReverbAudioProcessor : public juce::AudioProcessor
{
public:
    WarriorReverbAudioProcessor();
    ~WarriorReverbAudioProcessor() override;

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
    // Reverb algorithms
    enum ReverbType
    {
        Hall,
        Room,
        Plate,
        Spring,
        Shimmer
    };

    // DSP components
    struct ReverbEngine
    {
        std::vector<WarriorDSP::DelayLine> delayLines;
        std::vector<WarriorDSP::BiquadFilter> filters;
        std::vector<float> gains;
        
        void prepare(double sampleRate, int maxBlockSize);
        float processHall(float input, float roomSize, float damping, float diffusion);
        float processRoom(float input, float roomSize, float damping, float diffusion);
        float processPlate(float input, float roomSize, float damping, float diffusion);
        float processSpring(float input, float roomSize, float damping, float diffusion);
        float processShimmer(float input, float roomSize, float damping, float diffusion);
    };

    ReverbEngine reverbEngine;
    double currentSampleRate = 44100.0;
    
    // Parameter helpers
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarriorReverbAudioProcessor)
};