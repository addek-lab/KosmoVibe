#include "PluginProcessor.h"
#include "PluginEditor.h"

KosmoVibeAudioProcessor::KosmoVibeAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
}

KosmoVibeAudioProcessor::~KosmoVibeAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout KosmoVibeAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {KosmoParams::lfo_rate, 1}, "LFO Rate",
        juce::NormalisableRange<float>(0.02f, 100.0f, 0.01f, 0.3f), 5.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {KosmoParams::lfo_intensity, 1}, "LFO Int",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID {KosmoParams::lfo_shape, 1}, "LFO Shape",
        juce::StringArray{"Triangle/Ramp", "Pulse/PWM"}, 0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {KosmoParams::vcf_cutoff, 1}, "VCF Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 4000.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {KosmoParams::delay_time, 1}, "Delay Time",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f), 300.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {KosmoParams::delay_feedback, 1}, "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.2f, 0.01f), 0.5f));

    return layout;
}

const juce::String KosmoVibeAudioProcessor::getName() const { return JucePlugin_Name; }
bool KosmoVibeAudioProcessor::acceptsMidi() const { return true; }
bool KosmoVibeAudioProcessor::producesMidi() const { return false; }
bool KosmoVibeAudioProcessor::isMidiEffect() const { return false; }
double KosmoVibeAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int KosmoVibeAudioProcessor::getNumPrograms() { return 1; }
int KosmoVibeAudioProcessor::getCurrentProgram() { return 0; }
void KosmoVibeAudioProcessor::setCurrentProgram (int index) {}
const juce::String KosmoVibeAudioProcessor::getProgramName (int index) { return {}; }
void KosmoVibeAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void KosmoVibeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xmlState = apvts.copyState().createXml())
        copyXmlToBinary (*xmlState, destData);
}

void KosmoVibeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void KosmoVibeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Dynamically allocate delay buffer based on sample rate for max 1 second delay
    const float maxDelaySeconds = 1.0f;
    delayBuffer.setSize(1, static_cast<int>(std::ceil(sampleRate * maxDelaySeconds)));
    delayBuffer.clear();
    
    vco.prepare(sampleRate);
    lfo.prepare(sampleRate);
    vcf.prepare(sampleRate);
    delay.prepare(sampleRate);
    
    // 50ms smoothing for Cutoff and LFO intensity to prevent zipper noise
    smoothedCutoff.reset(sampleRate, 0.05);
    smoothedLfoInt.reset(sampleRate, 0.05);
    
    // 100ms smoothing for Delay Time to create organic analog warble when swept
    smoothedDelayTime.reset(sampleRate, 0.1);
}

void KosmoVibeAudioProcessor::releaseResources()
{
}

bool KosmoVibeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void KosmoVibeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // 1. Denormal-Numbers-Schutz (CPU-Spike-Prävention)
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
        
    // Target update for smoothers (read from APVTS once per block)
    smoothedCutoff.setTargetValue(apvts.getRawParameterValue(KosmoParams::vcf_cutoff)->load());
    smoothedLfoInt.setTargetValue(apvts.getRawParameterValue(KosmoParams::lfo_intensity)->load());
    smoothedDelayTime.setTargetValue(apvts.getRawParameterValue(KosmoParams::delay_time)->load());
    
    float lfoRate = apvts.getRawParameterValue(KosmoParams::lfo_rate)->load();
    int lfoShape = static_cast<int>(apvts.getRawParameterValue(KosmoParams::lfo_shape)->load());
    float delayFeedback = apvts.getRawParameterValue(KosmoParams::delay_feedback)->load();

    // Reading atomics for ribbon state (lock-free)
    float basePitch = ribbonPitchHz.load(std::memory_order_relaxed);
    bool gateOpen = ribbonGate.load(std::memory_order_relaxed);
    
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        // Smooth transition on sample level
        float currentCutoff = smoothedCutoff.getNextValue();
        float currentLfoInt = smoothedLfoInt.getNextValue();
        float currentDelayTime = smoothedDelayTime.getNextValue();

        // LFO
        float lfoVal = lfo.process(lfoRate, lfoShape);
        
        // Pitch modulation (1.0 intensity = +/- 1 octave shift)
        float modulatedPitch = basePitch * std::pow(2.0f, lfoVal * currentLfoInt);
        
        // VCO
        float vcoOut = 0.0f;
        if (gateOpen)
            vcoOut = vco.process(modulatedPitch);
        else
            vco.process(modulatedPitch); // Advance phase silently
            
        // VCF
        float vcfOut = vcf.process(vcoOut, currentCutoff);
        
        // Delay
        float delayOut = delay.process(vcfOut, currentDelayTime, delayFeedback);
        
        // Gain Staging & Master Soft-Clipping
        // Attenuate the incredibly hot analog sum by -6dB (0.5) and clip it softly
        // to prevent extreme digital distortion in the DAW.
        float masterOut = std::tanh(delayOut * 0.5f);
        
        // Stereo Output
        leftChannel[sample] = masterOut;
        if (rightChannel != nullptr)
            rightChannel[sample] = masterOut;
    }
}

bool KosmoVibeAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* KosmoVibeAudioProcessor::createEditor() { return new KosmoVibeAudioProcessorEditor (*this); }

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KosmoVibeAudioProcessor(); }
