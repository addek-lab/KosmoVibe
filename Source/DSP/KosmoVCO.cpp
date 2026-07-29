#include "KosmoVCO.h"
#include <algorithm>

void KosmoVCO::prepare(double sampleRate)
{
    fs = sampleRate;
    phase = 0.0f;
    lastParabolicSample = 0.0f;
}

float KosmoVCO::process(float frequencyHz)
{
    // Clamp strictly to Monotron's 4kHz limit
    frequencyHz = std::clamp(frequencyHz, 10.0f, 4000.0f);
    
    float deltaPhi = frequencyHz / static_cast<float>(fs);
    
    phase += deltaPhi;
    if (phase >= 1.0f) phase -= 1.0f;
    
    float s = 2.0f * phase - 1.0f;
    float p = s * s - (1.0f / 3.0f);
    
    float output = 0.0f;
    if (deltaPhi > 0.0001f)
        output = (p - lastParabolicSample) / (4.0f * deltaPhi);
    else
        output = s;
        
    lastParabolicSample = p;
    return output;
}
