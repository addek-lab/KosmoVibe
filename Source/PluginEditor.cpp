#include "PluginProcessor.h"
#include "PluginEditor.h"

KosmoVibeAudioProcessorEditor::KosmoVibeAudioProcessorEditor (KosmoVibeAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), ribbon(p)
{
    setSize (600, 320); // Compact Retro Layout
    
    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& name) {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setLookAndFeel(&customLookAndFeel);
        addAndMakeVisible(s);
        
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setColour(juce::Label::textColourId, juce::Colour(0xFFE0E0E0)); // Creme White
        addAndMakeVisible(l);
    };
    
    setupSlider(lfoRateSlider, lfoRateLabel, "LFO Rate");
    setupSlider(lfoIntSlider, lfoIntLabel, "LFO Int");
    setupSlider(vcfCutoffSlider, vcfCutoffLabel, "VCF Cutoff");
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time");
    setupSlider(delayFeedbackSlider, delayFeedbackLabel, "Feedback");
    
    lfoShapeToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xFFE0E0E0));
    lfoShapeToggle.setLookAndFeel(&customLookAndFeel);
    addAndMakeVisible(lfoShapeToggle);
    
    addAndMakeVisible(ribbon);
    
    // Create Attachments (RAII binds them to the APVTS and guarantees thread-safety)
    lfoRateAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, KosmoParams::lfo_rate, lfoRateSlider);
    lfoIntAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, KosmoParams::lfo_intensity, lfoIntSlider);
    vcfCutoffAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, KosmoParams::vcf_cutoff, vcfCutoffSlider);
    delayTimeAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, KosmoParams::delay_time, delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment>(audioProcessor.apvts, KosmoParams::delay_feedback, delayFeedbackSlider);
    lfoShapeAttachment = std::make_unique<ButtonAttachment>(audioProcessor.apvts, KosmoParams::lfo_shape, lfoShapeToggle);
}

KosmoVibeAudioProcessorEditor::~KosmoVibeAudioProcessorEditor()
{
    // Important: Clear LookAndFeel to prevent dangling pointers when UI is destroyed
    lfoRateSlider.setLookAndFeel(nullptr);
    lfoIntSlider.setLookAndFeel(nullptr);
    vcfCutoffSlider.setLookAndFeel(nullptr);
    delayTimeSlider.setLookAndFeel(nullptr);
    delayFeedbackSlider.setLookAndFeel(nullptr);
    lfoShapeToggle.setLookAndFeel(nullptr);
}

void KosmoVibeAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Matte Anthrazit/Schwarz background
    g.fillAll (juce::Colour(0xFF1A1A1A));
    
    // Decorative title
    g.setColour(juce::Colour(0xFFFF5500));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("KOSMOVIBE", getLocalBounds().removeFromTop(40).withTrimmedLeft(20), juce::Justification::centredLeft, true);
    
    g.setColour(juce::Colour(0xFFE0E0E0));
    g.setFont(juce::Font(14.0f));
    g.drawText("SPACE DELAY", getLocalBounds().removeFromTop(40).withTrimmedLeft(160), juce::Justification::centredLeft, true);
    
    // Add a divider line
    g.setColour(juce::Colour(0xFF333333));
    g.drawLine(20.0f, 40.0f, (float)getWidth() - 20.0f, 40.0f, 2.0f);
}

void KosmoVibeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    
    // Ribbon in the bottom area (100px height)
    ribbon.setBounds(area.removeFromBottom(110).reduced(20));
    
    // Top area for knobs
    auto topArea = area.removeFromTop(160).withTrimmedTop(50);
    
    int numKnobs = 5;
    int knobWidth = topArea.getWidth() / numKnobs;
    
    auto placeKnob = [&](juce::Slider& s, juce::Label& l, int index) {
        auto bounds = topArea.withWidth(knobWidth).withX(index * knobWidth);
        l.setBounds(bounds.removeFromBottom(30));
        s.setBounds(bounds.reduced(15)); // Add padding around the knob
    };
    
    placeKnob(lfoRateSlider, lfoRateLabel, 0);
    placeKnob(lfoIntSlider, lfoIntLabel, 1);
    placeKnob(vcfCutoffSlider, vcfCutoffLabel, 2);
    placeKnob(delayTimeSlider, delayTimeLabel, 3);
    placeKnob(delayFeedbackSlider, delayFeedbackLabel, 4);
    
    // Place LFO Shape Toggle below the LFO knobs
    lfoShapeToggle.setBounds(20, 190, 120, 24);
}
