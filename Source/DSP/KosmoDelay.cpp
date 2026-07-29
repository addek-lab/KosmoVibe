#include "KosmoDelay.h"

void KosmoDelay::prepare(double sampleRate, int maxDelaySecs)
{
    fs = sampleRate;
    bufferSize = static_cast<int>(sampleRate * maxDelaySecs);
    delayBuffer.setSize(1, bufferSize);
    delayBuffer.clear();
    writeIndex = 0;
}

float KosmoDelay::process(float input, float delayTimeMs, float feedbackAmt)
{
    float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(fs);
    
    float readIndexFloat = static_cast<float>(writeIndex) - delaySamples;
    if (readIndexFloat < 0.0f) readIndexFloat += static_cast<float>(bufferSize);
    
    int rIdx1 = static_cast<int>(std::floor(readIndexFloat)) % bufferSize;
    int rIdx2 = (rIdx1 + 1) % bufferSize;
    float frac = readIndexFloat - std::floor(readIndexFloat);
    
    float s1 = delayBuffer.getSample(0, rIdx1);
    float s2 = delayBuffer.getSample(0, rIdx2);
    float delayedSample = s1 + frac * (s2 - s1);
    
    // PT2399 Saturated Feedback loop
    float feedbackSample = std::tanh(delayedSample * feedbackAmt);
    
    delayBuffer.setSample(0, writeIndex, input + feedbackSample);
    
    writeIndex = (writeIndex + 1) % bufferSize;
    
    return input + delayedSample;
}
