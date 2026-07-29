#pragma once
#include <JuceHeader.h>

class KosmoDelay
{
public:
    void prepare(double sampleRate, int maxDelaySecs = 2);
    float process(float input, float delayTimeMs, float feedbackAmt);
private:
    double fs { 44100.0 };
    juce::AudioBuffer<float> delayBuffer;
    int bufferSize { 0 };
    int writeIndex { 0 };
};
