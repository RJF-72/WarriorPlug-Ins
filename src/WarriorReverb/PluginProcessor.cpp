#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorReverbAudioProcessor::WarriorReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
       parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

WarriorReverbAudioProcessor::~WarriorReverbAudioProcessor()
{
}

const juce::String WarriorReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarriorReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarriorReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarriorReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarriorReverbAudioProcessor::getTailLengthSeconds() const
{
    return 3.0; // 3 second reverb tail
}

int WarriorReverbAudioProcessor::getNumPrograms()
{
    return 5; // Number of reverb types
}

int WarriorReverbAudioProcessor::getCurrentProgram()
{
    return (int)*parameters.getRawParameterValue("reverbType");
}

void WarriorReverbAudioProcessor::setCurrentProgram (int index)
{
    *parameters.getRawParameterValue("reverbType") = (float)index;
}

const juce::String WarriorReverbAudioProcessor::getProgramName (int index)
{
    switch (index)
    {
        case Hall: return "Hall";
        case Room: return "Room";
        case Plate: return "Plate";
        case Spring: return "Spring";
        case Shimmer: return "Shimmer";
        default: return "Unknown";
    }
}

void WarriorReverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    // Not implemented for this plugin
}

void WarriorReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    reverbEngine.prepare(sampleRate, samplesPerBlock);
}

void WarriorReverbAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarriorReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void WarriorReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get parameters
    float roomSize = *parameters.getRawParameterValue("roomSize");
    float damping = *parameters.getRawParameterValue("damping");
    float diffusion = *parameters.getRawParameterValue("diffusion");
    float wetLevel = *parameters.getRawParameterValue("wetLevel");
    float dryLevel = *parameters.getRawParameterValue("dryLevel");
    int reverbType = (int)*parameters.getRawParameterValue("reverbType");

    auto numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = channelData[sample];
            float reverbSample = 0.0f;
            
            // Process through selected reverb algorithm
            switch (reverbType)
            {
                case Hall:
                    reverbSample = reverbEngine.processHall(inputSample, roomSize, damping, diffusion);
                    break;
                case Room:
                    reverbSample = reverbEngine.processRoom(inputSample, roomSize, damping, diffusion);
                    break;
                case Plate:
                    reverbSample = reverbEngine.processPlate(inputSample, roomSize, damping, diffusion);
                    break;
                case Spring:
                    reverbSample = reverbEngine.processSpring(inputSample, roomSize, damping, diffusion);
                    break;
                case Shimmer:
                    reverbSample = reverbEngine.processShimmer(inputSample, roomSize, damping, diffusion);
                    break;
            }
            
            // Mix dry and wet signals
            channelData[sample] = dryLevel * inputSample + wetLevel * reverbSample;
        }
    }
}

bool WarriorReverbAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* WarriorReverbAudioProcessor::createEditor()
{
    return new WarriorReverbAudioProcessorEditor (*this);
}

void WarriorReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WarriorReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout WarriorReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>("reverbType", "Reverb Type",
        juce::StringArray{"Hall", "Room", "Plate", "Spring", "Shimmer"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("roomSize", "Room Size",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("damping", "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("diffusion", "Diffusion",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetLevel", "Wet Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryLevel", "Dry Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

    return { params.begin(), params.end() };
}

// ReverbEngine implementation
void WarriorReverbAudioProcessor::ReverbEngine::prepare(double sampleRate, int maxBlockSize)
{
    const int numDelayLines = 8;
    delayLines.resize(numDelayLines);
    filters.resize(numDelayLines);
    gains.resize(numDelayLines);
    
    // Set up delay lines with different lengths for natural diffusion
    const std::vector<int> delayTimes = {347, 449, 571, 647, 751, 859, 997, 1151};
    
    for (int i = 0; i < numDelayLines; ++i)
    {
        delayLines[i].prepare(sampleRate, (int)(sampleRate * 3.0)); // 3 second max delay
        filters[i].reset();
        gains[i] = 0.7f - (i * 0.05f); // Decreasing gains
    }
}

float WarriorReverbAudioProcessor::ReverbEngine::processHall(float input, float roomSize, float damping, float diffusion)
{
    float output = 0.0f;
    
    for (size_t i = 0; i < delayLines.size(); ++i)
    {
        float delayTime = (347 + i * 100) * roomSize * 0.001f; // milliseconds to samples
        float feedback = 0.6f * (1.0f - damping);
        
        filters[i].setCoefficients(WarriorDSP::BiquadFilter::LowPass, 
                                 8000.0f * (1.0f - damping), 
                                 0.7f, 0.0f, 44100.0f);
        
        float delayed = delayLines[i].processSample(input + feedback * filters[i].processSample(output), 
                                                   delayTime * 44100.0f / 1000.0f, 
                                                   feedback);
        
        output += gains[i] * delayed * diffusion;
    }
    
    return output * 0.3f; // Scale output
}

float WarriorReverbAudioProcessor::ReverbEngine::processRoom(float input, float roomSize, float damping, float diffusion)
{
    return processHall(input, roomSize * 0.5f, damping, diffusion * 0.8f);
}

float WarriorReverbAudioProcessor::ReverbEngine::processPlate(float input, float roomSize, float damping, float diffusion)
{
    float output = 0.0f;
    
    // Plate reverb with shorter, denser reflections
    for (size_t i = 0; i < delayLines.size(); ++i)
    {
        float delayTime = (50 + i * 25) * roomSize * 0.001f;
        float feedback = 0.8f * (1.0f - damping);
        
        filters[i].setCoefficients(WarriorDSP::BiquadFilter::HighPass, 
                                 200.0f, 0.7f, 0.0f, 44100.0f);
        
        float delayed = delayLines[i].processSample(filters[i].processSample(input), 
                                                   delayTime * 44100.0f / 1000.0f, 
                                                   feedback);
        
        output += gains[i] * delayed * diffusion;
    }
    
    return output * 0.4f;
}

float WarriorReverbAudioProcessor::ReverbEngine::processSpring(float input, float roomSize, float damping, float diffusion)
{
    // Spring reverb with bouncy characteristics
    float delayed1 = delayLines[0].processSample(input, 100.0f * roomSize, 0.7f * (1.0f - damping));
    float delayed2 = delayLines[1].processSample(delayed1, 150.0f * roomSize, 0.6f * (1.0f - damping));
    
    // Add some modulation for spring character
    float modulation = std::sin(input * 1000.0f) * 0.002f;
    delayed2 += delayLines[2].processSample(input, (80.0f + modulation) * roomSize, 0.5f);
    
    return WarriorDSP::DSPUtils::softClip(delayed2 * diffusion * 0.5f);
}

float WarriorReverbAudioProcessor::ReverbEngine::processShimmer(float input, float roomSize, float damping, float diffusion)
{
    float hallReverb = processHall(input, roomSize, damping, diffusion);
    
    // Add octave up shimmer effect
    float shimmer = WarriorDSP::DSPUtils::softClip(hallReverb * 2.0f) * 0.2f;
    
    // High-pass filter the shimmer
    filters[0].setCoefficients(WarriorDSP::BiquadFilter::HighPass, 1000.0f, 0.7f, 0.0f, 44100.0f);
    shimmer = filters[0].processSample(shimmer);
    
    return hallReverb + shimmer;
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarriorReverbAudioProcessor();
}