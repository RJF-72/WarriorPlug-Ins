#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <functional>

namespace Warrior {

enum class GenreType {
    Rock,
    Jazz,
    Blues,
    Electronic,
    Classical,
    Country,
    Metal,
    Funk,
    Reggae,
    Pop,
    Hip_Hop,
    Folk,
    Custom
};

struct EffectParameter {
    std::string name;
    float value;
    float minValue;
    float maxValue;
    std::string unit;
    bool automatable;
};

class AudioEffect {
public:
    virtual ~AudioEffect() = default;
    virtual void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) = 0;
    virtual void setParameter(const std::string& paramName, float value) = 0;
    virtual float getParameter(const std::string& paramName) const = 0;
    virtual std::vector<EffectParameter> getParameters() const = 0;
    virtual void reset() = 0;
    virtual std::string getName() const = 0;
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};

// Specific effect implementations
class DistortionEffect : public AudioEffect {
public:
    DistortionEffect();
    void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) override;
    void setParameter(const std::string& paramName, float value) override;
    float getParameter(const std::string& paramName) const override;
    std::vector<EffectParameter> getParameters() const override;
    void reset() override;
    std::string getName() const override { return "Distortion"; }
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool enabled) override { this->enabled = enabled; }

private:
    bool enabled;
    float drive;
    float tone;
    float level;
    float dcBlockerState;
};

class ReverbEffect : public AudioEffect {
public:
    ReverbEffect();
    void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) override;
    void setParameter(const std::string& paramName, float value) override;
    float getParameter(const std::string& paramName) const override;
    std::vector<EffectParameter> getParameters() const override;
    void reset() override;
    std::string getName() const override { return "Reverb"; }
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool enabled) override { this->enabled = enabled; }

private:
    bool enabled;
    float roomSize;
    float damping;
    float wetLevel;
    float dryLevel;
    
    // Simple reverb delay lines
    std::vector<float> delayBuffer1;
    std::vector<float> delayBuffer2;
    std::vector<float> delayBuffer3;
    int delayIndex1, delayIndex2, delayIndex3;
};

class CompressorEffect : public AudioEffect {
public:
    CompressorEffect();
    void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) override;
    void setParameter(const std::string& paramName, float value) override;
    float getParameter(const std::string& paramName) const override;
    std::vector<EffectParameter> getParameters() const override;
    void reset() override;
    std::string getName() const override { return "Compressor"; }
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool enabled) override { this->enabled = enabled; }

private:
    bool enabled;
    float threshold;
    float ratio;
    float attack;
    float release;
    float makeupGain;
    float envelope;
};

class EQEffect : public AudioEffect {
public:
    EQEffect();
    void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels) override;
    void setParameter(const std::string& paramName, float value) override;
    float getParameter(const std::string& paramName) const override;
    std::vector<EffectParameter> getParameters() const override;
    void reset() override;
    std::string getName() const override { return "3-Band EQ"; }
    bool isEnabled() const override { return enabled; }
    void setEnabled(bool enabled) override { this->enabled = enabled; }

private:
    bool enabled;
    float lowGain;
    float midGain;
    float highGain;
    float lowFreq;
    float highFreq;
    
    // Biquad filter states
    float lowFilter[4];  // [b0, b1, a1, a2]
    float midFilter[4];
    float highFilter[4];
    float lowState[2];   // [x1, y1]
    float midState[2];
    float highState[2];
    
    void updateFilterCoefficients();
};

struct GenrePreset {
    GenreType genre;
    std::string name;
    std::string description;
    std::map<std::string, std::map<std::string, float>> effectSettings;
    std::vector<std::string> enabledEffects;
};

class GenreEffectsEngine {
public:
    GenreEffectsEngine();
    ~GenreEffectsEngine();

    // Genre management
    void setGenre(GenreType genre);
    GenreType getCurrentGenre() const { return currentGenre; }
    std::vector<GenreType> getAvailableGenres() const;
    std::string getGenreName(GenreType genre) const;
    
    // Effect processing
    void processAudio(float* inputBuffer, float* outputBuffer, int numSamples, int numChannels);
    
    // Effect chain management
    std::vector<std::shared_ptr<AudioEffect>> getEffectChain() const;
    void addEffect(std::shared_ptr<AudioEffect> effect);
    void removeEffect(const std::string& effectName);
    void reorderEffect(const std::string& effectName, int newPosition);
    
    // Parameter control
    void setEffectParameter(const std::string& effectName, const std::string& paramName, float value);
    float getEffectParameter(const std::string& effectName, const std::string& paramName) const;
    
    // Preset management
    void loadGenrePreset(GenreType genre);
    void saveCustomPreset(const std::string& name, GenreType genre);
    std::vector<GenrePreset> getGenrePresets() const;
    
    // Mix control
    void setDryWetMix(float mix); // 0.0 = fully dry, 1.0 = fully wet
    float getDryWetMix() const { return dryWetMix; }
    
    // Real-time analysis for automatic genre detection
    void enableAutoGenreDetection(bool enable) { autoDetectionEnabled = enable; }
    bool isAutoGenreDetectionEnabled() const { return autoDetectionEnabled; }
    GenreType analyzeAudioForGenre(const float* audioBuffer, int numSamples, int numChannels);

private:
    GenreType currentGenre;
    std::vector<std::shared_ptr<AudioEffect>> effectChain;
    std::vector<GenrePreset> genrePresets;
    float dryWetMix;
    bool autoDetectionEnabled;
    
    // Audio analysis for genre detection
    struct AudioAnalysis {
        float spectralCentroid;
        float spectralSpread;
        float zeroCrossingRate;
        float rmsEnergy;
        std::vector<float> mfccs;
    };
    
    AudioAnalysis analyzeAudioFeatures(const float* audioBuffer, int numSamples, int numChannels);
    GenreType classifyGenreFromFeatures(const AudioAnalysis& analysis);
    
    // Internal methods
    void initializeGenrePresets();
    void createDefaultEffectChain();
    float applyDryWetMix(float drySignal, float wetSignal) const;
};

} // namespace Warrior