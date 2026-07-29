#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "DSP/KosmoVCO.h"
#include "DSP/KosmoLFO.h"
#include "DSP/KosmoVCF.h"
#include "DSP/KosmoDelay.h"

namespace KosmoParams
{
    static constexpr auto lfo_rate       = "lfo_rate";
    static constexpr auto lfo_intensity  = "lfo_intensity";
    static constexpr auto lfo_shape      = "lfo_shape";
    static constexpr auto vcf_cutoff     = "vcf_cutoff";
    static constexpr auto delay_time     = "delay_time";
    static constexpr auto delay_feedback = "delay_feedback";
}

class KosmoVibeAudioProcessor  : public juce::AudioProcessor
{
public:
    KosmoVibeAudioProcessor();
    ~KosmoVibeAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // APVTS containing all plugin parameters
    juce::AudioProcessorValueTreeState apvts;

    // Thread-safe lock-free atomics for RibbonComponent -> Audio Thread communication
    std::atomic<float> ribbonPitchHz { 440.0f };
    std::atomic<bool> ribbonGate { false };

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // The delay buffer, dynamically sized in prepareToPlay
    juce::AudioBuffer<float> delayBuffer;
    
    // DSP Components
    KosmoVCO vco;
    KosmoLFO lfo;
    KosmoVCF vcf;
    KosmoDelay delay;
    
    // Parameter Smoothers for zipper-noise and warble prevention
    juce::LinearSmoothedValue<float> smoothedCutoff { 4000.0f };
    juce::LinearSmoothedValue<float> smoothedLfoInt { 0.5f };
    juce::LinearSmoothedValue<float> smoothedDelayTime { 300.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KosmoVibeAudioProcessor)
};
