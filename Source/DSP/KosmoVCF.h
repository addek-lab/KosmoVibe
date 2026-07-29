#pragma once
#include <JuceHeader.h>

class KosmoVCF
{
public:
    void prepare(double sampleRate);
    float process(float input, float cutoffHz);
private:
    double fs { 44100.0 };
    float s1 { 0.0f };
    float s2 { 0.0f };
};
