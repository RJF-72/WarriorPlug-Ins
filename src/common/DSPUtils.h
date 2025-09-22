#pragma once
#include <JuceHeader.h>

namespace WarriorDSP
{
    /**
     * Utility functions for digital signal processing
     */
    class DSPUtils
    {
    public:
        // Math utilities
        static float linearToDecibels(float linear);
        static float decibelsToLinear(float decibels);
        static float fastTanh(float x);
        
        // Filter utilities
        static void calculateBiquadCoefficients(float frequency, float q, float gain, float sampleRate, 
                                               float& b0, float& b1, float& b2, float& a1, float& a2);
        
        // Delay line utilities
        static int wrapDelayIndex(int index, int bufferSize);
        static float interpolateLinear(float sample1, float sample2, float fraction);
        static float interpolateCubic(float y0, float y1, float y2, float y3, float fraction);
        
        // Saturation/distortion utilities
        static float softClip(float input, float threshold = 0.7f);
        static float tubeModel(float input, float drive, float asymmetry = 0.0f);
        
        // Modulation utilities
        static float generateLFO(float phase, int waveform); // 0=sine, 1=triangle, 2=square, 3=saw
        
    private:
        DSPUtils() = delete; // Static utility class
    };
    
    /**
     * Simple delay line for audio processing
     */
    class DelayLine
    {
    public:
        DelayLine();
        ~DelayLine() = default;
        
        void prepare(double sampleRate, int maxDelayInSamples);
        void clear();
        void pushSample(float sample);
        float getDelayedSample(float delayInSamples);
        float processSample(float inputSample, float delayInSamples, float feedback = 0.0f);
        
    private:
        juce::AudioBuffer<float> buffer;
        int writeIndex = 0;
        int bufferSize = 0;
    };
    
    /**
     * Biquad filter implementation
     */
    class BiquadFilter
    {
    public:
        enum FilterType
        {
            LowPass,
            HighPass,
            BandPass,
            Notch,
            AllPass,
            LowShelf,
            HighShelf,
            Peak
        };
        
        BiquadFilter();
        void setCoefficients(FilterType type, float frequency, float q, float gain, float sampleRate);
        float processSample(float input);
        void reset();
        
    private:
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;
    };
}