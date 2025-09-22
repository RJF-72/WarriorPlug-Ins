#include "PluginProcessor.h"
#include "PluginEditor.h"

WarriorDelayAudioProcessor::WarriorDelayAudioProcessor()
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

WarriorDelayAudioProcessor::~WarriorDelayAudioProcessor()
{
}

const juce::String WarriorDelayAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarriorDelayAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarriorDelayAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarriorDelayAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarriorDelayAudioProcessor::getTailLengthSeconds() const
{
    return 2.0; // 2 second delay tail
}

int WarriorDelayAudioProcessor::getNumPrograms()
{
    return 1;
}

int WarriorDelayAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WarriorDelayAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WarriorDelayAudioProcessor::getProgramName (int index)
{
    return {};
}

void WarriorDelayAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void WarriorDelayAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay taps
    for (auto& tap : delayTaps)
    {
        tap.delayLine.prepare(sampleRate, (int)(sampleRate * 2.0)); // 2 second max delay
        tap.filter.reset();
    }
}

void WarriorDelayAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarriorDelayAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void WarriorDelayAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Get parameters
    float masterFeedback = *parameters.getRawParameterValue("masterFeedback");
    float wetLevel = *parameters.getRawParameterValue("wetLevel");
    float dryLevel = *parameters.getRawParameterValue("dryLevel");
    bool tempoSync = *parameters.getRawParameterValue("tempoSync") > 0.5f;
    float bpm = 120.0f; // Default BPM, could be obtained from host
    
    // Update LFO parameters
    lfoOscillators[0].frequency = *parameters.getRawParameterValue("lfo1Rate");
    lfoOscillators[0].depth = *parameters.getRawParameterValue("lfo1Depth");
    lfoOscillators[0].waveform = (int)*parameters.getRawParameterValue("lfo1Shape");
    
    lfoOscillators[1].frequency = *parameters.getRawParameterValue("lfo2Rate");
    lfoOscillators[1].depth = *parameters.getRawParameterValue("lfo2Depth");
    lfoOscillators[1].waveform = (int)*parameters.getRawParameterValue("lfo2Shape");
    
    // Update delay tap parameters
    for (int i = 0; i < 4; ++i)
    {
        juce::String paramPrefix = "tap" + juce::String(i + 1);
        
        delayTaps[i].enabled = *parameters.getRawParameterValue(paramPrefix + "Enable") > 0.5f;
        delayTaps[i].level = *parameters.getRawParameterValue(paramPrefix + "Level");
        delayTaps[i].feedback = *parameters.getRawParameterValue(paramPrefix + "Feedback");
        delayTaps[i].pan = *parameters.getRawParameterValue(paramPrefix + "Pan");
        
        if (tempoSync)
        {
            int syncIndex = (int)*parameters.getRawParameterValue(paramPrefix + "Time");
            delayTaps[i].delayTime = syncedDelayTimes[syncIndex] * (60.0f / bpm); // Convert to seconds
        }
        else
        {
            delayTaps[i].delayTime = *parameters.getRawParameterValue(paramPrefix + "Time");
        }
        
        // Set up filter
        float cutoff = *parameters.getRawParameterValue(paramPrefix + "Cutoff");
        float resonance = *parameters.getRawParameterValue(paramPrefix + "Resonance");
        delayTaps[i].filter.setCoefficients(WarriorDSP::BiquadFilter::LowPass, 
                                          cutoff, resonance, 0.0f, (float)currentSampleRate);
    }

    auto numSamples = buffer.getNumSamples();
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Update LFO phases
        for (auto& lfo : lfoOscillators)
        {
            lfo.updatePhase((float)currentSampleRate);
        }
        
        float lfo1Value = lfoOscillators[0].getValue();
        float lfo2Value = lfoOscillators[1].getValue();
        
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float inputSample = channelData[sample];
            float delaySum = 0.0f;
            
            // Process each delay tap
            for (int tapIndex = 0; tapIndex < 4; ++tapIndex)
            {
                if (!delayTaps[tapIndex].enabled)
                    continue;
                    
                auto& tap = delayTaps[tapIndex];
                
                // Apply modulation to delay time
                float modulatedDelayTime = tap.delayTime;
                if (tapIndex % 2 == 0) // LFO1 modulates taps 0 and 2
                    modulatedDelayTime += lfo1Value * 0.01f; // Small modulation amount
                else // LFO2 modulates taps 1 and 3
                    modulatedDelayTime += lfo2Value * 0.01f;
                
                // Get delayed sample with feedback
                float delaySamples = modulatedDelayTime * (float)currentSampleRate;
                float delayedSample = tap.delayLine.getDelayedSample(delaySamples);
                
                // Apply filter to feedback
                float filteredFeedback = tap.filter.processSample(delayedSample * tap.feedback * masterFeedback);
                
                // Push input + feedback to delay line
                tap.delayLine.pushSample(inputSample + filteredFeedback);
                
                // Apply panning (simple left/right for stereo)
                float panGain = 1.0f;
                if (totalNumInputChannels == 2)
                {
                    if (channel == 0) // Left channel
                        panGain = (tap.pan <= 0.0f) ? 1.0f : (1.0f - tap.pan);
                    else // Right channel
                        panGain = (tap.pan >= 0.0f) ? 1.0f : (1.0f + tap.pan);
                }
                
                delaySum += delayedSample * tap.level * panGain;
            }
            
            // Mix dry and wet signals
            channelData[sample] = dryLevel * inputSample + wetLevel * delaySum;
        }
    }
}

bool WarriorDelayAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* WarriorDelayAudioProcessor::createEditor()
{
    return new WarriorDelayAudioProcessorEditor (*this);
}

void WarriorDelayAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void WarriorDelayAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout WarriorDelayAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("masterFeedback", "Master Feedback",
        juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("wetLevel", "Wet Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryLevel", "Dry Level",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterBool>("tempoSync", "Tempo Sync", false));

    // LFO controls
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1Rate", "LFO 1 Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo1Depth", "LFO 1 Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo1Shape", "LFO 1 Shape",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2Rate", "LFO 2 Rate",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("lfo2Depth", "LFO 2 Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("lfo2Shape", "LFO 2 Shape",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw"}, 0));

    // Delay tap controls
    for (int i = 1; i <= 4; ++i)
    {
        juce::String paramPrefix = "tap" + juce::String(i);
        
        params.push_back(std::make_unique<juce::AudioParameterBool>(paramPrefix + "Enable", 
            "Tap " + juce::String(i) + " Enable", i <= 2)); // First 2 taps enabled by default
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Time", 
            "Tap " + juce::String(i) + " Time",
            juce::NormalisableRange<float>(0.01f, 2.0f, 0.01f), 0.25f * i));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Level", 
            "Tap " + juce::String(i) + " Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f - (i * 0.2f)));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Feedback", 
            "Tap " + juce::String(i) + " Feedback",
            juce::NormalisableRange<float>(0.0f, 0.95f, 0.01f), 0.1f));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Pan", 
            "Tap " + juce::String(i) + " Pan",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), (i % 2 == 1) ? -0.5f : 0.5f));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Cutoff", 
            "Tap " + juce::String(i) + " Cutoff",
            juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));
            
        params.push_back(std::make_unique<juce::AudioParameterFloat>(paramPrefix + "Resonance", 
            "Tap " + juce::String(i) + " Resonance",
            juce::NormalisableRange<float>(0.1f, 5.0f, 0.1f), 0.7f));
    }

    return { params.begin(), params.end() };
}

// ModulationOscillator implementation
void WarriorDelayAudioProcessor::ModulationOscillator::updatePhase(float sampleRate)
{
    phase += 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
    if (phase >= 2.0f * juce::MathConstants<float>::pi)
        phase -= 2.0f * juce::MathConstants<float>::pi;
}

float WarriorDelayAudioProcessor::ModulationOscillator::getValue()
{
    return depth * WarriorDSP::DSPUtils::generateLFO(phase, waveform);
}

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarriorDelayAudioProcessor();
}