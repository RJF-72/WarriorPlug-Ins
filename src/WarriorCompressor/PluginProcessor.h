#pragma once
#include <JuceHeader.h>
#include "../common/DSPUtils.h"

class WarriorCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    WarriorCompressorAudioProcessor();
    ~WarriorCompressorAudioProcessor() override;

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

    // For GUI meter display
    float getCurrentInputLevel() const { return currentInputLevel; }
    float getCurrentOutputLevel() const { return currentOutputLevel; }
    float getCurrentGainReduction() const { return currentGainReduction; }

private:
    // Compressor engine
    struct CompressorEngine
    {
        float envelope = 0.0f;
        float gainReduction = 0.0f;
        WarriorDSP::BiquadFilter sidechainFilter;
        WarriorDSP::BiquadFilter makeupFilter;
        
        void prepare(double sampleRate);
        float process(float input, float sidechainInput, 
                     float threshold, float ratio, float attack, float release,
                     float knee, float lookAhead, bool autoMakeup);
                     
    private:
        double sampleRate = 44100.0;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        
        void updateCoefficients(float attack, float release);
        float applyCompression(float input, float threshold, float ratio, float knee);
    };

    CompressorEngine compressorEngine;
    
    // Side-chain detection
    juce::AudioBuffer<float> sidechainBuffer;
    bool hasSidechainInput = false;
    
    // Look-ahead delay line
    WarriorDSP::DelayLine lookAheadDelay;
    
    // Level meters
    std::atomic<float> currentInputLevel{0.0f};
    std::atomic<float> currentOutputLevel{0.0f};
    std::atomic<float> currentGainReduction{0.0f};
    
    // Smoothed level detection
    float inputLevelSmooth = 0.0f;
    float outputLevelSmooth = 0.0f;
    
    double currentSampleRate = 44100.0;
    
    // Parameter helpers
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarriorCompressorAudioProcessor)
};