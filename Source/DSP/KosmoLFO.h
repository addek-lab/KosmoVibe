#pragma once
#include <JuceHeader.h>

class KosmoLFO
{
public:
    void prepare(double sampleRate);
    float process(float rateHz, int shapeMode);
private:
    float getTriangleRampSawBlend(float currentPhase, float morph);
    float getPulse(float currentPhase, float pulseWidth);
    
    double fs { 44100.0 };
    float phase { 0.0f };
};
