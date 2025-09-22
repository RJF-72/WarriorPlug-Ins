#include "DSPUtils.h"
#include <cmath>

namespace WarriorDSP
{
    // Math utilities
    float DSPUtils::linearToDecibels(float linear)
    {
        return linear > 0.0f ? 20.0f * std::log10(linear) : -100.0f;
    }
    
    float DSPUtils::decibelsToLinear(float decibels)
    {
        return decibels > -100.0f ? std::pow(10.0f, decibels * 0.05f) : 0.0f;
    }
    
    float DSPUtils::fastTanh(float x)
    {
        // Fast approximation of tanh
        if (x < -3.0f) return -1.0f;
        if (x > 3.0f) return 1.0f;
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }
    
    // Filter utilities
    void DSPUtils::calculateBiquadCoefficients(float frequency, float q, float gain, float sampleRate,
                                              float& b0, float& b1, float& b2, float& a1, float& a2)
    {
        float omega = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.0f * q);
        
        // Low-pass filter coefficients (can be extended for other types)
        b0 = (1.0f - cos_omega) / 2.0f;
        b1 = 1.0f - cos_omega;
        b2 = (1.0f - cos_omega) / 2.0f;
        a1 = -2.0f * cos_omega;
        a2 = 1.0f - alpha;
        
        // Normalize
        float a0 = 1.0f + alpha;
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
    
    // Delay utilities
    int DSPUtils::wrapDelayIndex(int index, int bufferSize)
    {
        if (index < 0)
            return index + bufferSize;
        if (index >= bufferSize)
            return index - bufferSize;
        return index;
    }
    
    float DSPUtils::interpolateLinear(float sample1, float sample2, float fraction)
    {
        return sample1 + fraction * (sample2 - sample1);
    }
    
    float DSPUtils::interpolateCubic(float y0, float y1, float y2, float y3, float fraction)
    {
        float a0 = y3 - y2 - y0 + y1;
        float a1 = y0 - y1 - a0;
        float a2 = y2 - y0;
        float a3 = y1;
        
        return a0 * fraction * fraction * fraction + a1 * fraction * fraction + a2 * fraction + a3;
    }
    
    // Saturation utilities
    float DSPUtils::softClip(float input, float threshold)
    {
        float abs_input = std::abs(input);
        if (abs_input <= threshold)
            return input;
        
        float sign = input > 0.0f ? 1.0f : -1.0f;
        float excess = abs_input - threshold;
        float compressed = threshold + excess / (1.0f + excess);
        
        return sign * compressed;
    }
    
    float DSPUtils::tubeModel(float input, float drive, float asymmetry)
    {
        float driven = input * drive;
        float asymmetric = driven + asymmetry * driven * driven;
        return fastTanh(asymmetric);
    }
    
    // Modulation utilities
    float DSPUtils::generateLFO(float phase, int waveform)
    {
        switch (waveform)
        {
            case 0: // Sine
                return std::sin(phase);
            case 1: // Triangle
                {
                    float normalizedPhase = phase / (2.0f * juce::MathConstants<float>::pi);
                    normalizedPhase = normalizedPhase - std::floor(normalizedPhase);
                    return normalizedPhase < 0.5f ? 4.0f * normalizedPhase - 1.0f : 3.0f - 4.0f * normalizedPhase;
                }
            case 2: // Square
                return std::sin(phase) > 0.0f ? 1.0f : -1.0f;
            case 3: // Sawtooth
                {
                    float normalizedPhase = phase / (2.0f * juce::MathConstants<float>::pi);
                    return 2.0f * (normalizedPhase - std::floor(normalizedPhase + 0.5f));
                }
            default:
                return 0.0f;
        }
    }
    
    // DelayLine implementation
    DelayLine::DelayLine() = default;
    
    void DelayLine::prepare(double sampleRate, int maxDelayInSamples)
    {
        bufferSize = maxDelayInSamples;
        buffer.setSize(1, bufferSize);
        clear();
    }
    
    void DelayLine::clear()
    {
        buffer.clear();
        writeIndex = 0;
    }
    
    void DelayLine::pushSample(float sample)
    {
        buffer.setSample(0, writeIndex, sample);
        writeIndex = (writeIndex + 1) % bufferSize;
    }
    
    float DelayLine::getDelayedSample(float delayInSamples)
    {
        float readPosition = writeIndex - delayInSamples;
        
        if (readPosition < 0.0f)
            readPosition += bufferSize;
            
        int readIndex = (int)readPosition;
        float fraction = readPosition - readIndex;
        
        int nextIndex = (readIndex + 1) % bufferSize;
        
        float sample1 = buffer.getSample(0, readIndex);
        float sample2 = buffer.getSample(0, nextIndex);
        
        return interpolateLinear(sample1, sample2, fraction);
    }
    
    float DelayLine::processSample(float inputSample, float delayInSamples, float feedback)
    {
        float delayedSample = getDelayedSample(delayInSamples);
        float outputSample = delayedSample;
        
        pushSample(inputSample + feedback * delayedSample);
        
        return outputSample;
    }
    
    // BiquadFilter implementation
    BiquadFilter::BiquadFilter() = default;
    
    void BiquadFilter::setCoefficients(FilterType type, float frequency, float q, float gain, float sampleRate)
    {
        float omega = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
        float sin_omega = std::sin(omega);
        float cos_omega = std::cos(omega);
        float alpha = sin_omega / (2.0f * q);
        float A = std::pow(10.0f, gain / 40.0f);
        
        float a0;
        
        switch (type)
        {
            case LowPass:
                b0 = (1.0f - cos_omega) / 2.0f;
                b1 = 1.0f - cos_omega;
                b2 = (1.0f - cos_omega) / 2.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_omega;
                a2 = 1.0f - alpha;
                break;
                
            case HighPass:
                b0 = (1.0f + cos_omega) / 2.0f;
                b1 = -(1.0f + cos_omega);
                b2 = (1.0f + cos_omega) / 2.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_omega;
                a2 = 1.0f - alpha;
                break;
                
            default: // Default to LowPass
                b0 = (1.0f - cos_omega) / 2.0f;
                b1 = 1.0f - cos_omega;
                b2 = (1.0f - cos_omega) / 2.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cos_omega;
                a2 = 1.0f - alpha;
                break;
        }
        
        // Normalize coefficients
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }
    
    float BiquadFilter::processSample(float input)
    {
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        return output;
    }
    
    void BiquadFilter::reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }
}