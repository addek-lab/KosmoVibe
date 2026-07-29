#pragma once

class KosmoVCO
{
public:
    void prepare(double sampleRate);
    float process(float frequencyHz);
private:
    double fs { 44100.0 };
    float phase { 0.0f };
    float lastParabolicSample { 0.0f };
};
