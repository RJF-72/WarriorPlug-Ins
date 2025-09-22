#pragma once
#include <JuceHeader.h>

namespace WarriorAudio
{
    /**
     * Custom look and feel for all Warrior plugins
     * Dark theme with warrior-inspired visual elements
     */
    class WarriorLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        WarriorLookAndFeel();
        ~WarriorLookAndFeel() override = default;
        
        // Color scheme
        struct Colors
        {
            static const juce::Colour background;
            static const juce::Colour surface;
            static const juce::Colour primary;
            static const juce::Colour secondary;
            static const juce::Colour accent;
            static const juce::Colour text;
            static const juce::Colour textSecondary;
            static const juce::Colour outline;
        };
        
        // Override key components
        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider& slider) override;
        
        void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, float minSliderPos, float maxSliderPos,
                            const juce::Slider::SliderStyle style, juce::Slider& slider) override;
        
        void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted,
                                bool shouldDrawButtonAsDown) override;
        
        juce::Font getLabelFont(juce::Label& label) override;
        
        void drawLabel(juce::Graphics& g, juce::Label& label) override;
        
        // Custom utility methods
        void drawWarriorFrame(juce::Graphics& g, juce::Rectangle<int> bounds, 
                            const juce::String& title = juce::String());
                            
        void drawMeterBar(juce::Graphics& g, juce::Rectangle<int> bounds,
                        float level, bool isHorizontal = false);
        
    private:
        juce::Font getWarriorFont(float height = 14.0f);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WarriorLookAndFeel)
    };
}