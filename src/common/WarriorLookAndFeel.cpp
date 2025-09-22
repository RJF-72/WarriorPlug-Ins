#include "WarriorLookAndFeel.h"

namespace WarriorAudio
{
    // Color definitions
    const juce::Colour WarriorLookAndFeel::Colors::background = juce::Colour(0xff1a1a1a);
    const juce::Colour WarriorLookAndFeel::Colors::surface = juce::Colour(0xff2d2d2d);
    const juce::Colour WarriorLookAndFeel::Colors::primary = juce::Colour(0xffff6b35);
    const juce::Colour WarriorLookAndFeel::Colors::secondary = juce::Colour(0xff4a9eff);
    const juce::Colour WarriorLookAndFeel::Colors::accent = juce::Colour(0xffffdd00);
    const juce::Colour WarriorLookAndFeel::Colors::text = juce::Colour(0xffffffff);
    const juce::Colour WarriorLookAndFeel::Colors::textSecondary = juce::Colour(0xffcccccc);
    const juce::Colour WarriorLookAndFeel::Colors::outline = juce::Colour(0xff555555);
    
    WarriorLookAndFeel::WarriorLookAndFeel()
    {
        // Set default colors
        setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
        setColour(juce::Label::textColourId, Colors::text);
        setColour(juce::Slider::backgroundColourId, Colors::surface);
        setColour(juce::Slider::thumbColourId, Colors::primary);
        setColour(juce::Slider::trackColourId, Colors::secondary);
        setColour(juce::TextButton::buttonColourId, Colors::surface);
        setColour(juce::TextButton::textColourOffId, Colors::text);
        setColour(juce::ComboBox::backgroundColourId, Colors::surface);
        setColour(juce::ComboBox::textColourId, Colors::text);
        setColour(juce::ComboBox::outlineColourId, Colors::outline);
    }
    
    void WarriorLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                            juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(10);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto centre = bounds.getCentre();
        
        // Draw outer ring
        g.setColour(Colors::surface);
        g.fillEllipse(bounds);
        
        // Draw outline
        g.setColour(Colors::outline);
        g.drawEllipse(bounds, 2.0f);
        
        // Draw track arc
        juce::Path trackArc;
        trackArc.addCentredArc(centre.x, centre.y, radius - 5, radius - 5,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(Colors::outline.withAlpha(0.5f));
        g.strokePath(trackArc, juce::PathStrokeType(3.0f));
        
        // Draw value arc
        if (sliderPos > 0.0f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, radius - 5, radius - 5,
                                  0.0f, rotaryStartAngle, toAngle, true);
            g.setColour(Colors::primary);
            g.strokePath(valueArc, juce::PathStrokeType(3.0f));
        }
        
        // Draw pointer
        juce::Path pointer;
        auto pointerLength = radius * 0.6f;
        auto pointerThickness = 3.0f;
        pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.7f);
        g.setColour(Colors::accent);
        g.fillPath(pointer, juce::AffineTransform::rotation(toAngle).translated(centre.x, centre.y));
    }
    
    void WarriorLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float minSliderPos, float maxSliderPos,
                                            const juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        auto trackBounds = juce::Rectangle<int>(x, y, width, height);
        
        if (slider.isHorizontal())
        {
            trackBounds.reduce(0, height / 3);
            
            // Draw track
            g.setColour(Colors::surface);
            g.fillRoundedRectangle(trackBounds.toFloat(), 3.0f);
            
            // Draw fill
            auto fillBounds = trackBounds;
            fillBounds.setWidth(sliderPos - x);
            g.setColour(Colors::primary);
            g.fillRoundedRectangle(fillBounds.toFloat(), 3.0f);
            
            // Draw thumb
            g.setColour(Colors::accent);
            g.fillEllipse(sliderPos - 8, y + height / 2 - 8, 16, 16);
        }
        else
        {
            trackBounds.reduce(width / 3, 0);
            
            // Draw track
            g.setColour(Colors::surface);
            g.fillRoundedRectangle(trackBounds.toFloat(), 3.0f);
            
            // Draw fill
            auto fillBounds = trackBounds;
            fillBounds.setTop(sliderPos);
            g.setColour(Colors::primary);
            g.fillRoundedRectangle(fillBounds.toFloat(), 3.0f);
            
            // Draw thumb
            g.setColour(Colors::accent);
            g.fillEllipse(x + width / 2 - 8, sliderPos - 8, 16, 16);
        }
    }
    
    void WarriorLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        
        auto baseColour = Colors::surface;
        if (shouldDrawButtonAsDown)
            baseColour = Colors::primary.darker();
        else if (shouldDrawButtonAsHighlighted)
            baseColour = Colors::surface.brighter(0.2f);
        
        g.setColour(baseColour);
        g.fillRoundedRectangle(bounds, 4.0f);
        
        g.setColour(Colors::outline);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
        
        if (button.getToggleState())
        {
            g.setColour(Colors::accent.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds.reduced(2.0f), 2.0f);
        }
    }
    
    juce::Font WarriorLookAndFeel::getLabelFont(juce::Label& label)
    {
        return getWarriorFont(label.getHeight() * 0.7f);
    }
    
    void WarriorLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
    {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));
        
        if (!label.isBeingEdited())
        {
            auto alpha = label.isEnabled() ? 1.0f : 0.5f;
            const auto textColour = label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha);
            
            g.setColour(textColour);
            g.setFont(getLabelFont(label));
            
            auto textArea = juce::Rectangle<int>(label.getBorderSize().subtractedFrom(label.getLocalBounds()));
            g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                           juce::jmax(1, (int)((float)textArea.getHeight() / getLabelFont(label).getHeight())),
                           label.getMinimumHorizontalScale());
        }
    }
    
    void WarriorLookAndFeel::drawWarriorFrame(juce::Graphics& g, juce::Rectangle<int> bounds,
                                            const juce::String& title)
    {
        g.setColour(Colors::surface);
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        
        g.setColour(Colors::outline);
        g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 2.0f);
        
        if (title.isNotEmpty())
        {
            g.setColour(Colors::primary);
            g.setFont(getWarriorFont(16.0f));
            g.drawText(title, bounds.getX() + 10, bounds.getY() + 5, 200, 20,
                      juce::Justification::centredLeft);
        }
    }
    
    void WarriorLookAndFeel::drawMeterBar(juce::Graphics& g, juce::Rectangle<int> bounds,
                                        float level, bool isHorizontal)
    {
        g.setColour(Colors::background);
        g.fillRect(bounds);
        
        level = juce::jlimit(0.0f, 1.0f, level);
        
        if (isHorizontal)
        {
            int fillWidth = (int)(bounds.getWidth() * level);
            auto fillBounds = bounds.withWidth(fillWidth);
            
            g.setColour(level > 0.8f ? juce::Colours::red : 
                       level > 0.6f ? juce::Colours::yellow : 
                       Colors::primary);
            g.fillRect(fillBounds);
        }
        else
        {
            int fillHeight = (int)(bounds.getHeight() * level);
            auto fillBounds = bounds.withTop(bounds.getBottom() - fillHeight).withHeight(fillHeight);
            
            g.setColour(level > 0.8f ? juce::Colours::red : 
                       level > 0.6f ? juce::Colours::yellow : 
                       Colors::primary);
            g.fillRect(fillBounds);
        }
        
        g.setColour(Colors::outline);
        g.drawRect(bounds, 1);
    }
    
    juce::Font WarriorLookAndFeel::getWarriorFont(float height)
    {
        return juce::Font(height, juce::Font::bold);
    }
}