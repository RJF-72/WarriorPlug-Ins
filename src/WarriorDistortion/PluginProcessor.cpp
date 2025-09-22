#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorDistortionAudioProcessor::WarriorDistortionAudioProcessor()
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

WarriorDistortionAudioProcessor::~WarriorDistortionAudioProcessor()
{
}

const juce::String WarriorDistortionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarriorDistortionAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarriorDistortionAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarriorDistortionAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarriorDistortionAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WarriorDistortionAudioProcessor::getNumPrograms()
{
    return 5; // Number of distortion types
}

int WarriorDistortionAudioProcessor::getCurrentProgram()
{
    return (int)*parameters.getRawParameterValue("stage1Type");
}

void WarriorDistortionAudioProcessor::setCurrentProgram (int index)
{
    *parameters.getRawParameterValue("stage1Type") = (float)index;
}

const juce::String WarriorDistortionAudioProcessor::getProgramName (int index)
{
    switch (index)
    {
        case Overdrive: return "Overdrive";
        case Fuzz: return "Fuzz";
        case Tube: return "Tube";
        case Bitcrush: return "Bitcrush";
        case Waveshaper: return "Waveshaper";
        default: return "Unknown";
    }
}

void WarriorDistortionAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    // Not implemented for this plugin
}

void WarriorDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare distortion stages
    for (auto& stage : distortionStages)
    {
        stage.prepare(sampleRate);
    }
    
    // Prepare tone stack
    bassFilter.reset();
    midFilter.reset();
    trebleFilter.reset();
    
    // Prepare cabinet simulation
    cabFilter1.reset();
    cabFilter2.reset();
}

void WarriorDistortionAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarriorDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void WarriorDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get parameters
    float inputGain = WarriorDSP::DSPUtils::decibelsToLinear(*parameters.getRawParameterValue("inputGain"));
    float outputGain = WarriorDSP::DSPUtils::decibelsToLinear(*parameters.getRawParameterValue("outputGain"));
    float asymmetry = *parameters.getRawParameterValue("asymmetry");
    
    // Distortion stage parameters
    for (int i = 0; i < 3; ++i)
    {
        juce::String stagePrefix = "stage" + juce::String(i + 1);
        
        distortionStages[i].enabled = *parameters.getRawParameterValue(stagePrefix + "Enable") > 0.5f;
        distortionStages[i].drive = *parameters.getRawParameterValue(stagePrefix + "Drive");
        distortionStages[i].gain = WarriorDSP::DSPUtils::decibelsToLinear(*parameters.getRawParameterValue(stagePrefix + "Gain"));
        distortionStages[i].type = (DistortionType)(int)*parameters.getRawParameterValue(stagePrefix + "Type");
    }
    
    // Tone stack parameters
    float bass = *parameters.getRawParameterValue("bass");
    float mid = *parameters.getRawParameterValue("mid");
    float treble = *parameters.getRawParameterValue("treble");
    
    // Cabinet simulation parameter
    bool cabEnabled = *parameters.getRawParameterValue("cabEnable") > 0.5f;
    float cabCutoff = *parameters.getRawParameterValue("cabCutoff");
    
    // Set up tone stack filters
    bassFilter.setCoefficients(WarriorDSP::BiquadFilter::LowShelf, 200.0f, 0.7f, bass, (float)currentSampleRate);
    midFilter.setCoefficients(WarriorDSP::BiquadFilter::Peak, 1000.0f, 0.7f, mid, (float)currentSampleRate);
    trebleFilter.setCoefficients(WarriorDSP::BiquadFilter::HighShelf, 5000.0f, 0.7f, treble, (float)currentSampleRate);
    
    // Set up cabinet simulation
    if (cabEnabled)
    {
        cabFilter1.setCoefficients(WarriorDSP::BiquadFilter::LowPass, cabCutoff, 0.7f, 0.0f, (float)currentSampleRate);
        cabFilter2.setCoefficients(WarriorDSP::BiquadFilter::LowPass, cabCutoff * 1.5f, 1.0f, 0.0f, (float)currentSampleRate);
    }

    auto numSamples = buffer.getNumSamples();
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float inputSample = channelData[sample] * inputGain;
            
            // Process through distortion stages
            float processedSample = inputSample;
            for (auto& stage : distortionStages)
            {
                if (stage.enabled)
                {
                    processedSample = stage.process(processedSample, asymmetry);
                }
            }
            
            // Apply tone stack
            processedSample = bassFilter.processSample(processedSample);
            processedSample = midFilter.processSample(processedSample);
            processedSample = trebleFilter.processSample(processedSample);
            
            // Apply cabinet simulation
            if (cabEnabled)
            {
                processedSample = cabFilter1.processSample(processedSample);
                processedSample = cabFilter2.processSample(processedSample);
            }
            
            // Apply output gain
            channelData[sample] = processedSample * outputGain;
        }
    }
}

bool WarriorDistortionAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* WarriorDistortionAudioProcessor::createEditor()
{
    return new WarriorDistortionAudioProcessorEditor (*this);
}

void WarriorDistortionAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WarriorDistortionAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout WarriorDistortionAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Input/Output
    params.push_back(std::make_unique<juce::AudioParameterFloat>("inputGain", "Input Gain",
        juce::NormalisableRange<float>(-20.0f, 40.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain",
        juce::NormalisableRange<float>(-40.0f, 20.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("asymmetry", "Asymmetry",
        juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

    // Distortion stages
    for (int i = 1; i <= 3; ++i)
    {
        juce::String stagePrefix = "stage" + juce::String(i);
        
        params.push_back(std::make_unique<juce::AudioParameterBool>(stagePrefix + "Enable", 
            "Stage " + juce::String(i) + " Enable", i == 1)); // Only first stage enabled by default
            
        params.push_back(std::make_unique<juce::AudioParameterChoice>(stagePrefix + "Type", 
            "Stage " + juce::String(i) + " Type",
            juce::StringArray{"Overdrive", "Fuzz", "Tube", "Bitcrush", "Waveshaper"}, 0));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(stagePrefix + "Drive", 
            "Stage " + juce::String(i) + " Drive",
            juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f), 5.0f));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(stagePrefix + "Gain", 
            "Stage " + juce::String(i) + " Gain",
            juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f));
    }

    // Tone stack
    params.push_back(std::make_unique<juce::AudioParameterFloat>("bass", "Bass",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("mid", "Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("treble", "Treble",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // Cabinet simulation
    params.push_back(std::make_unique<juce::AudioParameterBool>("cabEnable", "Cabinet Enable", false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("cabCutoff", "Cabinet Cutoff",
        juce::NormalisableRange<float>(2000.0f, 8000.0f, 10.0f), 5000.0f));

    return { params.begin(), params.end() };
}

// DistortionStage implementation
void WarriorDistortionAudioProcessor::DistortionStage::prepare(double sampleRate)
{
    preFilter.reset();
    postFilter.reset();
    
    // Set default pre and post filters
    preFilter.setCoefficients(WarriorDSP::BiquadFilter::HighPass, 80.0f, 0.7f, 0.0f, (float)sampleRate);
    postFilter.setCoefficients(WarriorDSP::BiquadFilter::LowPass, 8000.0f, 0.7f, 0.0f, (float)sampleRate);
}

float WarriorDistortionAudioProcessor::DistortionStage::process(float input, float asymmetry)
{
    // Pre-filter
    float filtered = preFilter.processSample(input);
    
    // Apply distortion based on type
    float distorted = 0.0f;
    switch (type)
    {
        case Overdrive:
            distorted = WarriorDSP::DSPUtils::softClip(filtered * drive, 0.7f);
            break;
        case Fuzz:
            distorted = WarriorDSP::DSPUtils::fastTanh(filtered * drive * 2.0f);
            break;
        case Tube:
            distorted = WarriorDSP::DSPUtils::tubeModel(filtered, drive, asymmetry);
            break;
        case Bitcrush:
            {
                float bits = juce::jmax(1.0f, drive);
                float levels = std::pow(2.0f, bits);
                distorted = std::floor(filtered * levels) / levels;
            }
            break;
        case Waveshaper:
            {
                float shaped = filtered * drive;
                distorted = shaped / (1.0f + std::abs(shaped));
            }
            break;
    }
    
    // Post-filter
    float output = postFilter.processSample(distorted);
    
    return output * gain;
}

// Distortion algorithm implementations
float WarriorDistortionAudioProcessor::processOverdrive(float input, float drive, float asymmetry)
{
    return WarriorDSP::DSPUtils::softClip(input * drive + asymmetry * input * input, 0.7f);
}

float WarriorDistortionAudioProcessor::processFuzz(float input, float drive, float asymmetry)
{
    float fuzzed = WarriorDSP::DSPUtils::fastTanh(input * drive * 3.0f);
    return fuzzed + asymmetry * fuzzed * fuzzed;
}

float WarriorDistortionAudioProcessor::processTube(float input, float drive, float asymmetry)
{
    return WarriorDSP::DSPUtils::tubeModel(input, drive, asymmetry);
}

float WarriorDistortionAudioProcessor::processBitcrush(float input, float drive, float asymmetry)
{
    float bits = juce::jmax(1.0f, 16.0f - drive);
    float levels = std::pow(2.0f, bits);
    float crushed = std::floor(input * levels) / levels;
    return crushed + asymmetry * crushed * 0.1f;
}

float WarriorDistortionAudioProcessor::processWaveshaper(float input, float drive, float asymmetry)
{
    float shaped = input * drive;
    float output = shaped / (1.0f + std::abs(shaped));
    return output + asymmetry * output * output * 0.5f;
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarriorDistortionAudioProcessor();
}