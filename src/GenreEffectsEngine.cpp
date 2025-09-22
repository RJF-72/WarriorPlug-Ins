#include "GenreEffectsEngine.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cstring>

namespace Warrior {

// DistortionEffect Implementation
DistortionEffect::DistortionEffect() : enabled(true), drive(0.5f), tone(0.5f), level(0.7f), dcBlockerState(0.0f) {}

void DistortionEffect::processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) {
    if (!enabled) {
        memcpy(outputBuffer, inputBuffer, numSamples * numChannels * sizeof(float));
        return;
    }
    
    float driveAmount = 1.0f + drive * 20.0f;
    float toneFilter = tone;
    
    for (int i = 0; i < numSamples * numChannels; i++) {
        float input = inputBuffer[i];
        
        // Apply drive
        float driven = input * driveAmount;
        
        // Soft clipping
        if (driven > 1.0f) driven = 1.0f - expf(driven - 1.0f);
        else if (driven < -1.0f) driven = -1.0f + expf(-driven - 1.0f);
        else driven = tanhf(driven);
        
        // Tone control (simple high-frequency roll-off)
        driven = driven * toneFilter + dcBlockerState * (1.0f - toneFilter);
        dcBlockerState = driven;
        
        outputBuffer[i] = driven * level;
    }
}

void DistortionEffect::setParameter(const std::string& paramName, float value) {
    if (paramName == "drive") drive = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "tone") tone = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "level") level = std::clamp(value, 0.0f, 1.0f);
}

float DistortionEffect::getParameter(const std::string& paramName) const {
    if (paramName == "drive") return drive;
    else if (paramName == "tone") return tone;
    else if (paramName == "level") return level;
    return 0.0f;
}

std::vector<EffectParameter> DistortionEffect::getParameters() const {
    return {
        {"drive", drive, 0.0f, 1.0f, "", true},
        {"tone", tone, 0.0f, 1.0f, "", true},
        {"level", level, 0.0f, 1.0f, "", true}
    };
}

void DistortionEffect::reset() {
    dcBlockerState = 0.0f;
}

// ReverbEffect Implementation
ReverbEffect::ReverbEffect() : enabled(true), roomSize(0.5f), damping(0.5f), wetLevel(0.3f), dryLevel(0.7f) {
    delayBuffer1.resize(2048, 0.0f);
    delayBuffer2.resize(3072, 0.0f);
    delayBuffer3.resize(4096, 0.0f);
    delayIndex1 = delayIndex2 = delayIndex3 = 0;
}

void ReverbEffect::processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) {
    if (!enabled) {
        memcpy(outputBuffer, inputBuffer, numSamples * numChannels * sizeof(float));
        return;
    }
    
    for (int i = 0; i < numSamples * numChannels; i++) {
        float input = inputBuffer[i];
        
        // Simple multi-tap delay reverb
        float delay1 = delayBuffer1[delayIndex1];
        float delay2 = delayBuffer2[delayIndex2];
        float delay3 = delayBuffer3[delayIndex3];
        
        delayBuffer1[delayIndex1] = input + delay1 * roomSize * damping;
        delayBuffer2[delayIndex2] = input + delay2 * roomSize * damping * 0.8f;
        delayBuffer3[delayIndex3] = input + delay3 * roomSize * damping * 0.6f;
        
        delayIndex1 = (delayIndex1 + 1) % delayBuffer1.size();
        delayIndex2 = (delayIndex2 + 1) % delayBuffer2.size();
        delayIndex3 = (delayIndex3 + 1) % delayBuffer3.size();
        
        float wet = (delay1 + delay2 + delay3) * 0.33f;
        outputBuffer[i] = input * dryLevel + wet * wetLevel;
    }
}

void ReverbEffect::setParameter(const std::string& paramName, float value) {
    if (paramName == "roomSize") roomSize = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "damping") damping = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "wetLevel") wetLevel = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "dryLevel") dryLevel = std::clamp(value, 0.0f, 1.0f);
}

float ReverbEffect::getParameter(const std::string& paramName) const {
    if (paramName == "roomSize") return roomSize;
    else if (paramName == "damping") return damping;
    else if (paramName == "wetLevel") return wetLevel;
    else if (paramName == "dryLevel") return dryLevel;
    return 0.0f;
}

std::vector<EffectParameter> ReverbEffect::getParameters() const {
    return {
        {"roomSize", roomSize, 0.0f, 1.0f, "", true},
        {"damping", damping, 0.0f, 1.0f, "", true},
        {"wetLevel", wetLevel, 0.0f, 1.0f, "", true},
        {"dryLevel", dryLevel, 0.0f, 1.0f, "", true}
    };
}

void ReverbEffect::reset() {
    std::fill(delayBuffer1.begin(), delayBuffer1.end(), 0.0f);
    std::fill(delayBuffer2.begin(), delayBuffer2.end(), 0.0f);
    std::fill(delayBuffer3.begin(), delayBuffer3.end(), 0.0f);
    delayIndex1 = delayIndex2 = delayIndex3 = 0;
}

// CompressorEffect Implementation
CompressorEffect::CompressorEffect() : enabled(true), threshold(0.7f), ratio(4.0f), attack(0.003f), release(0.1f), makeupGain(1.0f), envelope(0.0f) {}

void CompressorEffect::processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) {
    if (!enabled) {
        memcpy(outputBuffer, inputBuffer, numSamples * numChannels * sizeof(float));
        return;
    }
    
    for (int i = 0; i < numSamples * numChannels; i++) {
        float input = inputBuffer[i];
        float inputLevel = fabsf(input);
        
        // Envelope follower
        if (inputLevel > envelope) {
            envelope = inputLevel + (envelope - inputLevel) * expf(-attack);
        } else {
            envelope = inputLevel + (envelope - inputLevel) * expf(-release);
        }
        
        // Compression
        float gainReduction = 1.0f;
        if (envelope > threshold) {
            float overThreshold = envelope - threshold;
            gainReduction = 1.0f - (overThreshold * (ratio - 1.0f) / ratio);
        }
        
        outputBuffer[i] = input * gainReduction * makeupGain;
    }
}

void CompressorEffect::setParameter(const std::string& paramName, float value) {
    if (paramName == "threshold") threshold = std::clamp(value, 0.0f, 1.0f);
    else if (paramName == "ratio") ratio = std::clamp(value, 1.0f, 20.0f);
    else if (paramName == "attack") attack = std::clamp(value, 0.001f, 1.0f);
    else if (paramName == "release") release = std::clamp(value, 0.01f, 5.0f);
    else if (paramName == "makeupGain") makeupGain = std::clamp(value, 0.0f, 4.0f);
}

float CompressorEffect::getParameter(const std::string& paramName) const {
    if (paramName == "threshold") return threshold;
    else if (paramName == "ratio") return ratio;
    else if (paramName == "attack") return attack;
    else if (paramName == "release") return release;
    else if (paramName == "makeupGain") return makeupGain;
    return 0.0f;
}

std::vector<EffectParameter> CompressorEffect::getParameters() const {
    return {
        {"threshold", threshold, 0.0f, 1.0f, "", true},
        {"ratio", ratio, 1.0f, 20.0f, ":1", true},
        {"attack", attack, 0.001f, 1.0f, "s", true},
        {"release", release, 0.01f, 5.0f, "s", true},
        {"makeupGain", makeupGain, 0.0f, 4.0f, "x", true}
    };
}

void CompressorEffect::reset() {
    envelope = 0.0f;
}

// EQEffect Implementation
EQEffect::EQEffect() : enabled(true), lowGain(0.0f), midGain(0.0f), highGain(0.0f), lowFreq(250.0f), highFreq(4000.0f) {
    std::fill(lowFilter, lowFilter + 4, 0.0f);
    std::fill(midFilter, midFilter + 4, 0.0f);
    std::fill(highFilter, highFilter + 4, 0.0f);
    std::fill(lowState, lowState + 2, 0.0f);
    std::fill(midState, midState + 2, 0.0f);
    std::fill(highState, highState + 2, 0.0f);
    updateFilterCoefficients();
}

void EQEffect::processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) {
    if (!enabled) {
        memcpy(outputBuffer, inputBuffer, numSamples * numChannels * sizeof(float));
        return;
    }
    
    for (int i = 0; i < numSamples * numChannels; i++) {
        float input = inputBuffer[i];
        
        // Simple 3-band EQ using shelving filters
        float low = input * lowGain;
        float mid = input * midGain;
        float high = input * highGain;
        
        outputBuffer[i] = input + low + mid + high;
    }
}

void EQEffect::setParameter(const std::string& paramName, float value) {
    if (paramName == "lowGain") lowGain = std::clamp(value, -1.0f, 1.0f);
    else if (paramName == "midGain") midGain = std::clamp(value, -1.0f, 1.0f);
    else if (paramName == "highGain") highGain = std::clamp(value, -1.0f, 1.0f);
    else if (paramName == "lowFreq") lowFreq = std::clamp(value, 20.0f, 2000.0f);
    else if (paramName == "highFreq") highFreq = std::clamp(value, 1000.0f, 20000.0f);
    updateFilterCoefficients();
}

float EQEffect::getParameter(const std::string& paramName) const {
    if (paramName == "lowGain") return lowGain;
    else if (paramName == "midGain") return midGain;
    else if (paramName == "highGain") return highGain;
    else if (paramName == "lowFreq") return lowFreq;
    else if (paramName == "highFreq") return highFreq;
    return 0.0f;
}

std::vector<EffectParameter> EQEffect::getParameters() const {
    return {
        {"lowGain", lowGain, -1.0f, 1.0f, "dB", true},
        {"midGain", midGain, -1.0f, 1.0f, "dB", true},
        {"highGain", highGain, -1.0f, 1.0f, "dB", true},
        {"lowFreq", lowFreq, 20.0f, 2000.0f, "Hz", true},
        {"highFreq", highFreq, 1000.0f, 20000.0f, "Hz", true}
    };
}

void EQEffect::reset() {
    std::fill(lowState, lowState + 2, 0.0f);
    std::fill(midState, midState + 2, 0.0f);
    std::fill(highState, highState + 2, 0.0f);
}

void EQEffect::updateFilterCoefficients() {
    // Simplified filter coefficient calculation
    // In a real implementation, these would be proper biquad coefficients
}

// GenreEffectsEngine Implementation
GenreEffectsEngine::GenreEffectsEngine() : currentGenre(GenreType::Rock), dryWetMix(0.5f), autoDetectionEnabled(false) {
    initializeGenrePresets();
    createDefaultEffectChain();
}

GenreEffectsEngine::~GenreEffectsEngine() = default;

void GenreEffectsEngine::setGenre(GenreType genre) {
    currentGenre = genre;
    loadGenrePreset(genre);
}

std::vector<GenreType> GenreEffectsEngine::getAvailableGenres() const {
    return {
        GenreType::Rock, GenreType::Jazz, GenreType::Blues, GenreType::Electronic,
        GenreType::Classical, GenreType::Country, GenreType::Metal, GenreType::Funk,
        GenreType::Reggae, GenreType::Pop, GenreType::Hip_Hop, GenreType::Folk
    };
}

std::string GenreEffectsEngine::getGenreName(GenreType genre) const {
    switch (genre) {
        case GenreType::Rock: return "Rock";
        case GenreType::Jazz: return "Jazz";
        case GenreType::Blues: return "Blues";
        case GenreType::Electronic: return "Electronic";
        case GenreType::Classical: return "Classical";
        case GenreType::Country: return "Country";
        case GenreType::Metal: return "Metal";
        case GenreType::Funk: return "Funk";
        case GenreType::Reggae: return "Reggae";
        case GenreType::Pop: return "Pop";
        case GenreType::Hip_Hop: return "Hip-Hop";
        case GenreType::Folk: return "Folk";
        default: return "Custom";
    }
}

void GenreEffectsEngine::processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) {
    // Copy input to output first
    memcpy(outputBuffer, inputBuffer, numSamples * numChannels * sizeof(float));
    
    // Process through effect chain
    float* tempBuffer = new float[numSamples * numChannels];
    float* currentInput = outputBuffer;
    float* currentOutput = tempBuffer;
    
    for (auto& effect : effectChain) {
        if (effect->isEnabled()) {
            effect->processAudio(currentInput, currentOutput, numSamples, numChannels);
            std::swap(currentInput, currentOutput);
        }
    }
    
    // Apply dry/wet mix
    if (currentInput != outputBuffer) {
        for (int i = 0; i < numSamples * numChannels; i++) {
            outputBuffer[i] = applyDryWetMix(inputBuffer[i], currentInput[i]);
        }
    }
    
    delete[] tempBuffer;
    
    // Auto genre detection if enabled
    if (autoDetectionEnabled) {
        GenreType detectedGenre = analyzeAudioForGenre(inputBuffer, numSamples, numChannels);
        if (detectedGenre != currentGenre) {
            // Could trigger genre change or just notify
            std::cout << "Detected genre change to: " << getGenreName(detectedGenre) << std::endl;
        }
    }
}

void GenreEffectsEngine::initializeGenrePresets() {
    // Rock preset
    GenrePreset rockPreset;
    rockPreset.genre = GenreType::Rock;
    rockPreset.name = "Rock";
    rockPreset.description = "Classic rock sound with distortion and reverb";
    rockPreset.effectSettings["Distortion"] = {{"drive", 0.6f}, {"tone", 0.7f}, {"level", 0.8f}};
    rockPreset.effectSettings["Reverb"] = {{"roomSize", 0.4f}, {"wetLevel", 0.2f}};
    rockPreset.effectSettings["Compressor"] = {{"threshold", 0.6f}, {"ratio", 3.0f}};
    rockPreset.enabledEffects = {"Distortion", "Compressor", "3-Band EQ", "Reverb"};
    genrePresets.push_back(rockPreset);
    
    // Jazz preset
    GenrePreset jazzPreset;
    jazzPreset.genre = GenreType::Jazz;
    jazzPreset.name = "Jazz";
    jazzPreset.description = "Warm, clean jazz tone with subtle compression";
    jazzPreset.effectSettings["Compressor"] = {{"threshold", 0.8f}, {"ratio", 2.0f}};
    jazzPreset.effectSettings["Reverb"] = {{"roomSize", 0.6f}, {"wetLevel", 0.3f}};
    jazzPreset.effectSettings["3-Band EQ"] = {{"midGain", 0.2f}, {"highGain", -0.1f}};
    jazzPreset.enabledEffects = {"3-Band EQ", "Compressor", "Reverb"};
    genrePresets.push_back(jazzPreset);
    
    // Metal preset
    GenrePreset metalPreset;
    metalPreset.genre = GenreType::Metal;
    metalPreset.name = "Metal";
    metalPreset.description = "High-gain distortion with tight compression";
    metalPreset.effectSettings["Distortion"] = {{"drive", 0.9f}, {"tone", 0.8f}, {"level", 0.9f}};
    metalPreset.effectSettings["Compressor"] = {{"threshold", 0.5f}, {"ratio", 6.0f}};
    metalPreset.effectSettings["3-Band EQ"] = {{"lowGain", 0.3f}, {"highGain", 0.4f}};
    metalPreset.enabledEffects = {"3-Band EQ", "Distortion", "Compressor"};
    genrePresets.push_back(metalPreset);
}

void GenreEffectsEngine::createDefaultEffectChain() {
    effectChain.clear();
    effectChain.push_back(std::make_shared<EQEffect>());
    effectChain.push_back(std::make_shared<DistortionEffect>());
    effectChain.push_back(std::make_shared<CompressorEffect>());
    effectChain.push_back(std::make_shared<ReverbEffect>());
}

void GenreEffectsEngine::loadGenrePreset(GenreType genre) {
    auto presetIt = std::find_if(genrePresets.begin(), genrePresets.end(),
        [genre](const GenrePreset& preset) { return preset.genre == genre; });
    
    if (presetIt != genrePresets.end()) {
        const GenrePreset& preset = *presetIt;
        
        // Disable all effects first
        for (auto& effect : effectChain) {
            effect->setEnabled(false);
        }
        
        // Apply preset settings
        for (const auto& effectName : preset.enabledEffects) {
            for (auto& effect : effectChain) {
                if (effect->getName() == effectName) {
                    effect->setEnabled(true);
                    
                    // Apply parameters if they exist for this effect
                    auto settingsIt = preset.effectSettings.find(effectName);
                    if (settingsIt != preset.effectSettings.end()) {
                        for (const auto& param : settingsIt->second) {
                            effect->setParameter(param.first, param.second);
                        }
                    }
                    break;
                }
            }
        }
        
        std::cout << "Loaded genre preset: " << preset.name << std::endl;
    }
}

float GenreEffectsEngine::applyDryWetMix(float drySignal, float wetSignal) const {
    return drySignal * (1.0f - dryWetMix) + wetSignal * dryWetMix;
}

GenreType GenreEffectsEngine::analyzeAudioForGenre(const float* audioBuffer, int numSamples, int numChannels) {
    AudioAnalysis analysis = analyzeAudioFeatures(audioBuffer, numSamples, numChannels);
    return classifyGenreFromFeatures(analysis);
}

GenreEffectsEngine::AudioAnalysis GenreEffectsEngine::analyzeAudioFeatures(const float* audioBuffer, int numSamples, int numChannels) {
    AudioAnalysis analysis;
    
    // Calculate RMS energy
    float sumSquares = 0.0f;
    for (int i = 0; i < numSamples * numChannels; i++) {
        sumSquares += audioBuffer[i] * audioBuffer[i];
    }
    analysis.rmsEnergy = sqrtf(sumSquares / (numSamples * numChannels));
    
    // Calculate zero crossing rate
    int zeroCrossings = 0;
    for (int i = 1; i < numSamples * numChannels; i++) {
        if ((audioBuffer[i] >= 0) != (audioBuffer[i-1] >= 0)) {
            zeroCrossings++;
        }
    }
    analysis.zeroCrossingRate = static_cast<float>(zeroCrossings) / (numSamples * numChannels);
    
    // Simplified spectral features (would need FFT for real implementation)
    analysis.spectralCentroid = analysis.zeroCrossingRate * 1000.0f; // Rough approximation
    analysis.spectralSpread = analysis.rmsEnergy * 500.0f; // Rough approximation
    
    return analysis;
}

GenreType GenreEffectsEngine::classifyGenreFromFeatures(const AudioAnalysis& analysis) {
    // Simplified genre classification based on audio features
    if (analysis.rmsEnergy > 0.8f && analysis.zeroCrossingRate > 0.15f) {
        return GenreType::Metal;
    } else if (analysis.zeroCrossingRate < 0.05f && analysis.rmsEnergy < 0.3f) {
        return GenreType::Jazz;
    } else if (analysis.rmsEnergy > 0.6f && analysis.zeroCrossingRate > 0.1f) {
        return GenreType::Rock;
    } else if (analysis.spectralCentroid > 2000.0f) {
        return GenreType::Electronic;
    } else {
        return GenreType::Pop; // Default fallback
    }
}

std::vector<std::shared_ptr<AudioEffect>> GenreEffectsEngine::getEffectChain() const {
    return effectChain;
}

void GenreEffectsEngine::setEffectParameter(const std::string& effectName, const std::string& paramName, float value) {
    for (auto& effect : effectChain) {
        if (effect->getName() == effectName) {
            effect->setParameter(paramName, value);
            break;
        }
    }
}

float GenreEffectsEngine::getEffectParameter(const std::string& effectName, const std::string& paramName) const {
    for (const auto& effect : effectChain) {
        if (effect->getName() == effectName) {
            return effect->getParameter(paramName);
        }
    }
    return 0.0f;
}

void GenreEffectsEngine::setDryWetMix(float mix) {
    dryWetMix = std::clamp(mix, 0.0f, 1.0f);
}

} // namespace Warrior