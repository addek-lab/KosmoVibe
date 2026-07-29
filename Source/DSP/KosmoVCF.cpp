#include "KosmoVCF.h"

void KosmoVCF::prepare(double sampleRate)
{
    fs = sampleRate;
    s1 = 0.0f;
    s2 = 0.0f;
}

float KosmoVCF::process(float input, float cutoffHz)
{
    float wc = 2.0f * juce::MathConstants<float>::pi * cutoffHz;
    float T = 1.0f / static_cast<float>(fs);
    float g = wc * T / 2.0f;
    
    float k = 1.0f; // Fixed resonance characteristic of Monotron Delay
    
    float v3 = input - s2;
    float v1 = (g * v3 + s1) / (1.0f + g * (g + k));
    
    float saturated_v1 = std::tanh(v1); // Non-linear distortion simulation
    
    float v2 = g * saturated_v1 + s2;
    
    s1 = 2.0f * saturated_v1 - s1;
    s2 = 2.0f * v2 - s2;
    
    return v2;
}
