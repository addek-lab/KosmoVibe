#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <optional>

class RibbonComponent : public juce::Component
{
public:
    RibbonComponent(KosmoVibeAudioProcessor& p) : processor(p)
    {
    }

    void paint(juce::Graphics& g) override
    {
        // Matte dark background
        g.fillAll(juce::Colour(0xFF1A1A1A));
        
        // Draw decorative "keys" to hint at a keyboard
        g.setColour(juce::Colour(0xFFE0E0E0).withAlpha(0.15f));
        int numKeys = 30;
        float keyWidth = getWidth() / (float)numKeys;
        for (int i = 0; i < numKeys; ++i) {
            g.drawRect(i * keyWidth, 0.0f, keyWidth, (float)getHeight(), 1.0f);
        }
        
        // Frame
        g.setColour(juce::Colour(0xFF333333));
        g.drawRect(getLocalBounds(), 2.0f);
        
        // Watermark "by Addek-Labs"
        g.setColour(juce::Colour(0xFF666666)); // Subtle grey
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("by Addek-Labs", getLocalBounds().reduced(8), juce::Justification::bottomRight, false);

        // Draw orange touch point
        if (touchPosition.has_value()) {
            g.setColour(juce::Colour(0xFFFF5500)); // Korg Orange
            float x = touchPosition.value() * getWidth();
            g.fillEllipse(x - 6.0f, getHeight() / 2.0f - 6.0f, 12.0f, 12.0f);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        handleMouse(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        handleMouse(e);
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        processor.ribbonGate.store(false, std::memory_order_relaxed);
        touchPosition.reset();
        repaint();
    }

private:
    void handleMouse(const juce::MouseEvent& e)
    {
        // Normalize X position (0.0 to 1.0)
        float normalizedX = juce::jlimit(0.0f, 1.0f, e.x / (float)getWidth());
        touchPosition = normalizedX;
        
        // Exponential pitch scale: 50 Hz to 4000 Hz
        float minFreq = 50.0f;
        float maxFreq = 4000.0f;
        float freq = minFreq * std::pow(maxFreq / minFreq, normalizedX);
        
        processor.ribbonPitchHz.store(freq, std::memory_order_relaxed);
        processor.ribbonGate.store(true, std::memory_order_relaxed);
        repaint();
    }

    KosmoVibeAudioProcessor& processor;
    std::optional<float> touchPosition;
};
