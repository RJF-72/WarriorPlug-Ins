#pragma once
#include <JuceHeader.h>
#include "../common/DSPUtils.h"

class WarriorDistortionAudioProcessor : public juce::AudioProcessor
{
public:
    WarriorDistortionAudioProcessor();
    ~WarriorDistortionAudioProcessor() override;

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
    // Distortion types
    enum DistortionType
    {
        Overdrive,
        Fuzz,
        Tube,
        Bitcrush,
        Waveshaper
    };

    // Multi-stage distortion engine
    struct DistortionStage
    {
        WarriorDSP::BiquadFilter preFilter;
        WarriorDSP::BiquadFilter postFilter;
        float drive = 1.0f;
        float gain = 1.0f;
        DistortionType type = Overdrive;
        bool enabled = true;
        
        void prepare(double sampleRate);
        float process(float input, float asymmetry = 0.0f);
    };

    std::array<DistortionStage, 3> distortionStages; // 3-stage distortion
    
    // Tone stack
    WarriorDSP::BiquadFilter bassFilter;
    WarriorDSP::BiquadFilter midFilter;
    WarriorDSP::BiquadFilter trebleFilter;
    
    // Cabinet simulation
    WarriorDSP::BiquadFilter cabFilter1;
    WarriorDSP::BiquadFilter cabFilter2;
    
    double currentSampleRate = 44100.0;
    
    // Parameter helpers
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Distortion algorithms
    float processOverdrive(float input, float drive, float asymmetry);
    float processFuzz(float input, float drive, float asymmetry);
    float processTube(float input, float drive, float asymmetry);
    float processBitcrush(float input, float drive, float asymmetry);
    float processWaveshaper(float input, float drive, float asymmetry);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarriorDistortionAudioProcessor)
};