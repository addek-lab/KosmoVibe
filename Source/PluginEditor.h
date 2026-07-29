#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "RibbonComponent.h"

// Custom LookAndFeel for Monotron style
class KosmoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KosmoLookAndFeel()
    {
        setColour(juce::Slider::thumbColourId, juce::Colour(0xFFFF5500)); // Korg Orange
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFFF5500));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xFF333333));
        setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFFF5500));
        setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xFF333333));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 4.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Fill background
        g.setColour (juce::Colour(0xFF2A2A2A));
        g.fillEllipse (rx, ry, rw, rw);

        // Outline
        g.setColour (juce::Colour(0xFF111111));
        g.drawEllipse (rx, ry, rw, rw, 2.0f);

        // Orange Pointer
        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 3.0f;
        p.addRectangle (-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform (juce::AffineTransform::rotation (angle).translated (centreX, centreY));
        g.setColour (juce::Colour(0xFFFF5500));
        g.fillPath (p);
    }
};

class KosmoVibeAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    KosmoVibeAudioProcessorEditor (KosmoVibeAudioProcessor&);
    ~KosmoVibeAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KosmoVibeAudioProcessor& audioProcessor;
    
    KosmoLookAndFeel customLookAndFeel;
    RibbonComponent ribbon;
    
    juce::Slider lfoRateSlider;
    juce::Slider lfoIntSlider;
    juce::Slider vcfCutoffSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;
    juce::ToggleButton lfoShapeToggle { "Pulse (PWM)" };
    
    juce::Label lfoRateLabel { {}, "LFO Rate" };
    juce::Label lfoIntLabel { {}, "LFO Int" };
    juce::Label vcfCutoffLabel { {}, "VCF Cutoff" };
    juce::Label delayTimeLabel { {}, "Delay Time" };
    juce::Label delayFeedbackLabel { {}, "Feedback" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Attachments must be declared AFTER sliders to ensure RAII destruction order
    std::unique_ptr<SliderAttachment> lfoRateAttachment;
    std::unique_ptr<SliderAttachment> lfoIntAttachment;
    std::unique_ptr<SliderAttachment> vcfCutoffAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<ButtonAttachment> lfoShapeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KosmoVibeAudioProcessorEditor)
};
