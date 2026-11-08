#pragma once

// Small DSP building blocks shared by all engines. Plain C++ so the offline render tool
// can use the synth without JUCE.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace bk
{

constexpr float pi = 3.14159265358979f;
constexpr float twoPi = 6.28318530717959f;

inline float midiToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// Per-sample multiplier that decays by 1/e in tau seconds
inline float decayCoef (float tauSeconds, float sr) { return std::exp (-1.0f / (std::max (1.0e-4f, tauSeconds) * sr)); }

inline float onePoleCoef (float hz, float sr) { return 1.0f - std::exp (-twoPi * hz / sr); }

// Rational tanh approximation, accurate enough for saturation stages
inline float fastTanh (float x)
{
    x = std::clamp (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Transparent below the knee, smooth ceiling at 1
inline float softClip (float x)
{
    constexpr float knee = 0.8f;
    const float a = std::abs (x);
    if (a <= knee)
        return x;
    return std::copysign (knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee)), x);
}

struct Rng
{
    explicit Rng (uint32_t seed = 22222) : state (seed) {}

    // uniform in [-1, 1)
    float bipolar()
    {
        state = state * 1664525u + 1013904223u;
        return (float) (state >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    float unipolar() { return 0.5f * (bipolar() + 1.0f); }

    uint32_t state;
};

// Band-limited step correction for saw/square oscillators
inline float polyBlep (float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Topology-preserving state variable filter (Zavalishin)
struct Svf
{
    void set (float hz, float q, float sr)
    {
        g = std::tan (pi * std::min (hz, 0.48f * sr) / sr);
        k = 1.0f / std::max (0.1f, q);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    void process (float v0, float& lp, float& bp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = v0 - k * v1 - v2;
    }

    float lowpass (float x) { float l, b, h; process (x, l, b, h); return l; }
    float bandpass (float x) { float l, b, h; process (x, l, b, h); return b; }
    float highpass (float x) { float l, b, h; process (x, l, b, h); return h; }

    void reset() { ic1 = ic2 = 0.0f; }

    float g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0, ic1 = 0, ic2 = 0;
};

// Four one-pole stages with saturation in each, resonance by global feedback (Moog-style ladder)
struct Ladder
{
    void set (float hz, float resonance, float sr)
    {
        g = onePoleCoef (std::clamp (hz, 10.0f, 0.42f * sr), sr);
        k = 3.9f * std::clamp (resonance, 0.0f, 1.0f);
    }

    float process (float x)
    {
        const float in = fastTanh (x * (1.0f + 0.5f * k) - k * s[3]);
        s[0] += g * (in - fastTanh (s[0]));
        s[1] += g * (fastTanh (s[0]) - fastTanh (s[1]));
        s[2] += g * (fastTanh (s[1]) - fastTanh (s[2]));
        s[3] += g * (fastTanh (s[2]) - fastTanh (s[3]));
        return s[3];
    }

    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    float s[4] {}, g = 0, k = 0;
};

struct DcBlocker
{
    float process (float x)
    {
        const float y = x - prevIn + 0.995f * prevOut;
        prevIn = x;
        prevOut = y;
        return y;
    }

    void reset() { prevIn = prevOut = 0.0f; }

    float prevIn = 0, prevOut = 0;
};

// Linear attack, exponential decay/release
struct Adsr
{
    enum class Stage { idle, attack, decay, sustain, release };

    void set (float attackS, float decayS, float sustainLevel, float releaseS, float sr)
    {
        attackInc = 1.0f / std::max (1.0f, attackS * sr);
        decayMul = decayCoef (decayS, sr);
        sustain = sustainLevel;
        releaseMul = decayCoef (releaseS, sr);
    }

    void noteOn() { stage = Stage::attack; }
    void noteOff() { if (stage != Stage::idle) stage = Stage::release; }

    float process()
    {
        switch (stage)
        {
            case Stage::attack:
                value += attackInc;
                if (value >= 1.0f) { value = 1.0f; stage = Stage::decay; }
                break;
            case Stage::decay:
                value = sustain + (value - sustain) * decayMul;
                if (value - sustain < 1.0e-4f) stage = Stage::sustain;
                break;
            case Stage::sustain:
                value = sustain;
                break;
            case Stage::release:
                value *= releaseMul;
                if (value < 1.0e-5f) { value = 0.0f; stage = Stage::idle; }
                break;
            case Stage::idle:
                break;
        }
        return value;
    }

    Stage stage = Stage::idle;
    float value = 0, attackInc = 1, decayMul = 0, sustain = 1, releaseMul = 0;
};

} // namespace bk
