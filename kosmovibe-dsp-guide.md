# KosmoVibe - VST DSP Development Guide

This guide describes the mathematical formulations, DSP theory, and concrete C++ code structures needed to model the analog circuits of the **KosmoVibe** plugin (Korg Monotron Delay clone) inside the JUCE Framework.

---

## 1. Voltage Controlled Oscillator (VCO)

The KosmoVibe oscillator generates a **sawtooth waveform** with a hard frequency ceiling of **4,000 Hz**. [41]

### Aliasing Prevention (DPW - Differentiated Parabolic Wave)
Simply generating a naive sawtooth using `fmod(phase, 1.0)` will introduce severe high-frequency digital distortion (aliasing) in a C++ digital environment. To mimic analog smoothness, use the **Differentiated Parabolic Wave (DPW)** algorithm:

$$\text{Naive Sawtooth: } s(t) = 2 \cdot \phi(t) - 1$$
$$\text{Parabolic Wave: } p(t) = s(t)^2 - \frac{1}{3} = 4\phi(t)^2 - 4\phi(t) + \frac{2}{3}$$
$$\text{Differentiated Output: } y(t) = \frac{p(t) - p(t-T)}{4 \cdot \Delta \phi}$$

Where $\Delta \phi = \frac{f_{\text{VCO}}}{f_s}$ and $f_s$ is the sample rate.

### C++ DSP Blueprint
```cpp
class KosmoVibeVCO 
{
public:
    void prepare(double sampleRate) {
        fs = sampleRate;
        phase = 0.0;
        lastParabolicSample = 0.0;
    }

    float process(float frequencyHz) {
        // Clamp VCO frequency to the Monotron Delay's 4kHz ceiling [41]
        frequencyHz = std::clamp(frequencyHz, 10.0f, 4000.0f);
        
        float deltaPhi = frequencyHz / static_cast<float>(fs);
        
        // Phase accumulator update
        phase += deltaPhi;
        if (phase >= 1.0f) phase -= 1.0f;
        
        // DPW Processing
        float s = 2.0f * phase - 1.0f;
        float p = s * s - (1.0f / 3.0f);
        
        // Prevent division by zero if frequency is static/zero
        float output = 0.0f;
        if (deltaPhi > 0.0001f) {
            output = (p - lastParabolicSample) / (4.0f * deltaPhi);
        } else {
            output = s;
        }
        
        lastParabolicSample = p;
        return output;
    }

private:
    double fs { 44100.0 };
    float phase { 0.0f };
    float lastParabolicSample { 0.0f };
};
```

---

## 2. Low Frequency Oscillator (LFO)

The LFO on the Monotron Delay features extreme capabilities:
*   Frequencies down to **0.02 Hz**. [41]
*   Modulation target is permanently hard-routed to **oscillator pitch** (VCO frequency). [41]
*   Waveform modes:
    1.  **Triangle / Ramp / Sawtooth Blend:** Stufenlos (smoothly) morphing from a rising ramp to a symmetric triangle to a falling sawtooth. [41]
    2.  **Pulse with PWM:** Symmetric pulse morphing into narrow-width PWM. [41]

### Waveform Blend Derivation
Let the morph parameter be $M \in [0.0, 1.0]$ where $M=0$ is Ramp, $M=0.5$ is Triangle, and $M=1$ is Sawtooth.

Given phase $\theta \in [0.0, 1.0)$:
```cpp
float getTriangleRampSawBlend(float phase, float morph) {
    // Determine the peak point based on morph parameter
    float peak = juce::jmap(morph, 0.0f, 1.0f, 0.01f, 0.99f);
    
    if (phase < peak) {
        return (phase / peak) * 2.0f - 1.0f;
    } else {
        return ((1.0f - phase) / (1.0f - peak)) * 2.0f - 1.0f;
    }
}
```

---

## 3. MS-20 12 dB/Octave Low-Pass Filter (VCF)

The Monotron Delay uses the legendary MS-20 Sallen-Key low-pass filter design. [36, 41] Although the original MS-20 is resonant, the Monotron Delay only exposes **VCF Cutoff** control. [41]

### Virtuell-Analoge Sallen-Key Topologie (Zero-Delay Feedback)
To capture the iconic screaming analog character of the MS-20 VCF, avoid simple digital bi-quad filters (which lack the non-linear saturation characteristics) and implement a **Zero-Delay Feedback (ZDF)** Sallen-Key model.

The basic structure for an analog-modeled 2-pole Sallen-Key lowpass uses a feedback loop with internal clipping:

```cpp
class KosmoVibeVCF 
{
public:
    void prepare(double sampleRate) {
        fs = sampleRate;
        s1 = 0.0f;
        s2 = 0.0f;
    }

    float process(float input, float cutoffHz) {
        float wc = 2.0f * juce::MathConstants<float>::pi * cutoffHz;
        float T = 1.0f / static_cast<float>(fs);
        float g = wc * T / 2.0f; // bilinear transform coefficient
        
        // Fixed resonance parameter to match Monotron Delay default feedback behavior
        float k = 1.0f; 
        
        // Solve the feedback loop using a zero-delay feedback approximation
        // Including a non-linear saturation step inside the filter loop:
        float v3 = input - s2;
        float v1 = (g * v3 + s1) / (1.0f + g * (g + k));
        
        // Non-linear distortion simulation
        float saturated_v1 = std::tanh(v1); 
        
        float v2 = g * saturated_v1 + s2;
        
        // State updates
        s1 = 2.0f * saturated_v1 - s1;
        s2 = 2.0f * v2 - s2;
        
        return v2; // Low-pass filter output
    }

private:
    double fs { 44100.0 };
    float s1 { 0.0f }, s2 { 0.0f };
};
```

---

## 4. PT2399 Space Delay Emulation

The "Space Delay" is modeled after the **PT2399** digital echo processor chip. [41] It provides:
*   Delay times up to **1 second (1000.0 ms)**. [41]
*   **Feedback control** that reaches **self-oscillation** at high values. [41]

### Self-Oscillation & Feedback Saturation
To prevent feedback levels $> 1.0$ from causing numerical overflows and clipping, the feedback path must contain a saturating non-linearity (`std::tanh`). This keeps the signal bounded while producing beautiful analog tape-like saturation when in self-oscillation.

### fractional Delay via Linear Interpolation
To allow smooth, pitch-shifting tape-warble effects when modifying the Delay Time in real-time, read from the delay buffer using **fractional interpolation**.

```cpp
class KosmoVibeDelay 
{
public:
    void prepare(double sampleRate, int maxDelaySecs = 2) {
        fs = sampleRate;
        bufferSize = static_cast<int>(sampleRate * maxDelaySecs);
        delayBuffer.setSize(1, bufferSize);
        delayBuffer.clear();
        writeIndex = 0;
    }

    float process(float input, float delayTimeMs, float feedbackAmt) {
        // Calculate target delay in samples
        float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(fs);
        
        // Find read indices for interpolation
        float readIndexFloat = static_cast<float>(writeIndex) - delaySamples;
        if (readIndexFloat < 0.0f) readIndexFloat += static_cast<float>(bufferSize);
        
        int rIdx1 = static_cast<int>(std::floor(readIndexFloat)) % bufferSize;
        int rIdx2 = (rIdx1 + 1) % bufferSize;
        float frac = readIndexFloat - std::floor(readIndexFloat);
        
        // Perform linear interpolation
        float s1 = delayBuffer.getSample(0, rIdx1);
        float s2 = delayBuffer.getSample(0, rIdx2);
        float delayedSample = s1 + frac * (s2 - s1);
        
        // PT2399 Saturated Feedback loop
        // Standard high feedback leads to warm saturation without digital clipping
        float feedbackSample = std::tanh(delayedSample * feedbackAmt);
        
        // Write back to buffer
        delayBuffer.setSample(0, writeIndex, input + feedbackSample);
        
        // Increment index
        writeIndex = (writeIndex + 1) % bufferSize;
        
        // Combine Dry/Wet mix (Monotron Space Delay is permanently mixed with the main VCF output)
        return input + delayedSample;
    }

private:
    double fs { 44100.0 };
    juce::AudioBuffer<float> delayBuffer;
    int bufferSize { 0 };
    int writeIndex { 0 };
};
```
