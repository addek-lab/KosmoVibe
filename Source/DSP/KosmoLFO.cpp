#include "KosmoLFO.h"

void KosmoLFO::prepare(double sampleRate)
{
    fs = sampleRate;
    phase = 0.0f;
}

float KosmoLFO::process(float rateHz, int shapeMode)
{
    float deltaPhi = rateHz / static_cast<float>(fs);
    phase += deltaPhi;
    if (phase >= 1.0f) phase -= 1.0f;
    
    // For now, morph is fixed to perfect Triangle (0.5) and perfect Square (0.5)
    if (shapeMode == 0)
        return getTriangleRampSawBlend(phase, 0.5f);
    else
        return getPulse(phase, 0.5f);
}

float KosmoLFO::getTriangleRampSawBlend(float currentPhase, float morph)
{
    float peak = juce::jmap(morph, 0.0f, 1.0f, 0.01f, 0.99f);
    if (currentPhase < peak)
        return (currentPhase / peak) * 2.0f - 1.0f;
    else
        return ((1.0f - currentPhase) / (1.0f - peak)) * 2.0f - 1.0f;
}

float KosmoLFO::getPulse(float currentPhase, float pulseWidth)
{
    return (currentPhase < pulseWidth) ? 1.0f : -1.0f;
}
