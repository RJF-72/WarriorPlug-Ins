#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorCompressorAudioProcessor::WarriorCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
       parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

WarriorCompressorAudioProcessor::~WarriorCompressorAudioProcessor()
{
}

const juce::String WarriorCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarriorCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarriorCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarriorCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarriorCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WarriorCompressorAudioProcessor::getNumPrograms()
{
    return 1;
}

int WarriorCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WarriorCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WarriorCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void WarriorCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void WarriorCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    compressorEngine.prepare(sampleRate);
    
    // Prepare look-ahead delay (5ms max)
    lookAheadDelay.prepare(sampleRate, (int)(sampleRate * 0.005));
    
    // Prepare sidechain buffer
    sidechainBuffer.setSize(2, samplesPerBlock);
    
    // Reset level meters
    currentInputLevel = 0.0f;
    currentOutputLevel = 0.0f;
    currentGainReduction = 0.0f;
    inputLevelSmooth = 0.0f;
    outputLevelSmooth = 0.0f;
}

void WarriorCompressorAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarriorCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main input/output should be stereo or mono
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    // Sidechain can be stereo, mono, or disabled
    auto sidechainLayout = layouts.getChannelSet(true, 1);
    if (!sidechainLayout.isDisabled() && sidechainLayout != juce::AudioChannelSet::mono() 
        && sidechainLayout != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

void WarriorCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Check for sidechain input
    hasSidechainInput = getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
    
    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get parameters
    float threshold = *parameters.getRawParameterValue("threshold");
    float ratio = *parameters.getRawParameterValue("ratio");
    float attack = *parameters.getRawParameterValue("attack");
    float release = *parameters.getRawParameterValue("release");
    float knee = *parameters.getRawParameterValue("knee");
    float makeupGain = WarriorDSP::DSPUtils::decibelsToLinear(*parameters.getRawParameterValue("makeupGain"));
    float wetLevel = *parameters.getRawParameterValue("wetLevel");
    float lookAheadTime = *parameters.getRawParameterValue("lookAhead");
    bool autoMakeup = *parameters.getRawParameterValue("autoMakeup") > 0.5f;
    bool sidechainEnable = *parameters.getRawParameterValue("sidechainEnable") > 0.5f && hasSidechainInput;

    auto numSamples = buffer.getNumSamples();
    
    // Prepare sidechain buffer
    if (hasSidechainInput && sidechainEnable)
    {
        sidechainBuffer.copyFrom(0, 0, buffer, 2, 0, numSamples); // Copy sidechain input
        if (buffer.getNumChannels() > 3)
            sidechainBuffer.copyFrom(1, 0, buffer, 3, 0, numSamples);
        else
            sidechainBuffer.copyFrom(1, 0, sidechainBuffer, 0, 0, numSamples); // Mono sidechain
    }

    // Process each channel
    for (int channel = 0; channel < juce::jmin(totalNumInputChannels, 2); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        float maxInputLevel = 0.0f;
        float maxOutputLevel = 0.0f;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = channelData[sample];
            float sidechainSample = inputSample;
            
            // Use sidechain if available and enabled
            if (hasSidechainInput && sidechainEnable)
            {
                sidechainSample = sidechainBuffer.getSample(juce::jmin(channel, sidechainBuffer.getNumChannels() - 1), sample);
            }
            
            // Process through compressor
            float compressedSample = compressorEngine.process(inputSample, sidechainSample,
                                                            threshold, ratio, attack, release,
                                                            knee, lookAheadTime, autoMakeup);
            
            // Apply makeup gain (if not auto)
            if (!autoMakeup)
                compressedSample *= makeupGain;
            
            // Mix wet/dry
            float outputSample = wetLevel * compressedSample + (1.0f - wetLevel) * inputSample;
            
            channelData[sample] = outputSample;
            
            // Track levels for metering
            maxInputLevel = juce::jmax(maxInputLevel, std::abs(inputSample));
            maxOutputLevel = juce::jmax(maxOutputLevel, std::abs(outputSample));
        }
        
        // Update level meters (only for first channel to avoid redundant updates)
        if (channel == 0)
        {
            float smoothingFactor = 0.99f;
            inputLevelSmooth = inputLevelSmooth * smoothingFactor + maxInputLevel * (1.0f - smoothingFactor);
            outputLevelSmooth = outputLevelSmooth * smoothingFactor + maxOutputLevel * (1.0f - smoothingFactor);
            
            currentInputLevel = inputLevelSmooth;
            currentOutputLevel = outputLevelSmooth;
            currentGainReduction = compressorEngine.gainReduction;
        }
    }
}

bool WarriorCompressorAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* WarriorCompressorAudioProcessor::createEditor()
{
    return new WarriorCompressorAudioProcessorEditor (*this);
}

void WarriorCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WarriorCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout WarriorCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Main compressor parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>("threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -12.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("ratio", "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.3f), 4.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f), 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("release", "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.3f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("knee", "Knee",
        juce::NormalisableRange<float>(0.0f, 12.0f, 0.1f), 2.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("makeupGain", "Makeup Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetLevel", "Wet/Dry",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("lookAhead", "Look Ahead",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("autoMakeup", "Auto Makeup", true));

    // Sidechain parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>("sidechainEnable", "Sidechain Enable", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("sidechainHPF", "Sidechain HPF",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.3f), 80.0f));

    return { params.begin(), params.end() };
}

// CompressorEngine implementation
void WarriorCompressorAudioProcessor::CompressorEngine::prepare(double sampleRate)
{
    this->sampleRate = sampleRate;
    sidechainFilter.reset();
    makeupFilter.reset();
    envelope = 0.0f;
    gainReduction = 0.0f;
}

float WarriorCompressorAudioProcessor::CompressorEngine::process(float input, float sidechainInput,
                                                                float threshold, float ratio, float attack, float release,
                                                                float knee, float lookAhead, bool autoMakeup)
{
    // Update envelope coefficients if needed
    updateCoefficients(attack, release);
    
    // Filter sidechain input
    float detectionSignal = sidechainFilter.processSample(sidechainInput);
    
    // Calculate level in dB
    float inputLevelDB = WarriorDSP::DSPUtils::linearToDecibels(std::abs(detectionSignal));
    
    // Calculate compression
    float targetGainReduction = 0.0f;
    if (inputLevelDB > threshold)
    {
        targetGainReduction = applyCompression(inputLevelDB, threshold, ratio, knee) - inputLevelDB;
    }
    
    // Smooth gain reduction with attack/release
    if (targetGainReduction < gainReduction) // Attacking (reducing gain)
        gainReduction = gainReduction * attackCoeff + targetGainReduction * (1.0f - attackCoeff);
    else // Releasing (increasing gain)
        gainReduction = gainReduction * releaseCoeff + targetGainReduction * (1.0f - releaseCoeff);
    
    // Convert gain reduction back to linear
    float gainLinear = WarriorDSP::DSPUtils::decibelsToLinear(gainReduction);
    
    // Apply auto makeup gain
    if (autoMakeup && ratio > 1.0f)
    {
        float autoGainDB = (threshold - threshold / ratio) * 0.5f; // Rough auto makeup calculation
        gainLinear *= WarriorDSP::DSPUtils::decibelsToLinear(autoGainDB);
    }
    
    return input * gainLinear;
}

void WarriorCompressorAudioProcessor::CompressorEngine::updateCoefficients(float attack, float release)
{
    // Convert ms to coefficients
    attackCoeff = std::exp(-1.0f / (attack * 0.001f * sampleRate));
    releaseCoeff = std::exp(-1.0f / (release * 0.001f * sampleRate));
}

float WarriorCompressorAudioProcessor::CompressorEngine::applyCompression(float input, float threshold, float ratio, float knee)
{
    if (knee <= 0.0f)
    {
        // Hard knee
        return threshold + (input - threshold) / ratio;
    }
    else
    {
        // Soft knee
        float kneeStart = threshold - knee * 0.5f;
        float kneeEnd = threshold + knee * 0.5f;
        
        if (input < kneeStart)
        {
            return input;
        }
        else if (input > kneeEnd)
        {
            return threshold + (input - threshold) / ratio;
        }
        else
        {
            // Interpolate in knee region
            float kneeRatio = (input - kneeStart) / knee;
            float targetRatio = 1.0f + (ratio - 1.0f) * kneeRatio * kneeRatio;
            return kneeStart + (input - kneeStart) / targetRatio;
        }
    }
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarriorCompressorAudioProcessor();
}