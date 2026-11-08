#pragma once

#include <juce_dsp/juce_dsp.h>

namespace birch
{

// Each era is a different "machine": how low the sample rate / bit depth
// can go at full Grit, how dark the output filter gets, and how much vinyl noise.
struct EraSettings
{
    float minRate;    // sample rate at Grit = 100 %
    float minBits;    // bit depth at Grit = 100 %
    float toneHz;     // output low-pass at Grit = 100 %
    float preFilter;  // anti-alias filter relative to the new Nyquist (>1 lets aliasing through)
    float hiss;
    float crackle;
};

inline constexpr EraSettings eras[] = {
    { 12000.0f,  8.0f,  7000.0f, 1.30f, 1.00f, 1.30f },   // '88 - dirty and dark
    { 22050.0f, 12.0f, 11000.0f, 1.00f, 0.70f, 1.00f },   // '94 - classic 12-bit
    { 32000.0f, 14.0f, 15000.0f, 0.85f, 0.40f, 0.60f },   // '98 - cleaner, heavier
};

// 45 rpm record played at 33 1/3: 12 * log2 (33.333 / 45)
inline constexpr float rpm45to33Semitones = -5.2f;

struct Params
{
    float grit = 0, knock = 0, thump = 0, warmth = 0, dust = 0, wobble = 0;
    float pitch = 0;        // semitones
    float lowCutHz = 20;
    float width = 1;        // 0 = mono, 1 = unchanged, 2 = extra wide
    float mix = 1, outDb = 0;
    int era = 1;
    bool monoBass = true, sidechain = false, bypass = false;
};

struct Envelope
{
    void setTimes (double sr, float attackMs, float releaseMs)
    {
        attack = coefficient (sr, attackMs);
        release = coefficient (sr, releaseMs);
    }

    float process (float x)
    {
        const float c = x > env ? attack : release;
        env = x + c * (env - x);
        return env;
    }

    static float coefficient (double sr, float ms) { return (float) std::exp (-1.0 / (0.001 * ms * sr)); }

    float env = 0, attack = 0, release = 0;
};

class Engine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    int getLatencySamples() const { return baseDelay; }

    // sidechain may be null; it is only listened to when Params::sidechain is on
    void process (juce::AudioBuffer<float>& buffer, const Params& p, const juce::AudioBuffer<float>* sidechain);

    // Largest Knock gain reduction in the last processed block, in dB (positive)
    float getGainReductionDb() const { return gainReductionDb; }

private:
    float noiseSample (int ch, float hissLevel, float crackleLevel, float crackleProb);

    double sr = 44100.0;
    int baseDelay = 0;
    float gainReductionDb = 0;

    // Low cut before everything, so Thump doesn't pump up sub rumble
    juce::dsp::StateVariableTPTFilter<float> lowCut;

    // Pitch: two crossfaded taps sweeping through a delay line
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> pitchDelay { 8192 };
    double pitchPhase = 0;
    int pitchWindow = 0;
    juce::SmoothedValue<float> pitchBlend;

    // Thump: low band is saturated separately; dry path gets the same all-pass so Mix doesn't comb.
    juce::dsp::LinkwitzRileyFilter<float> thumpXover, dryXover;

    // Knock: transient shaper + linked compressor
    Envelope fastEnv, slowEnv, compEnv;

    // Grit: sample & hold + quantiser between two filters
    juce::dsp::StateVariableTPTFilter<float> preFilter, toneFilter;
    double holdPhase = 0;
    float held[2] {};

    // Warmth: asymmetric tape saturation, head-loss low-pass and DC blocker
    float tapeLp[2] {}, dcIn[2] {}, dcOut[2] {};

    // Wobble: modulated delay; dry path gets the fixed base delay
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> wobbleDelay { 4096 }, dryDelay { 4096 };
    double wowPhase = 0, flutterPhase = 0;
    float drift = 0, driftTarget = 0;

    // Width / mono bass in mid-side; dry gets the matching all-pass
    juce::dsp::LinkwitzRileyFilter<float> msXover, dryMsXover;
    juce::SmoothedValue<float> widthSmoothed;

    // Dust
    Envelope duckEnv;
    juce::Random rng;
    float hissLp[2] {}, crackleEnv[2] {}, crackleHpPrev[2] {}, crackleHpOut[2] {};
    float crackleDecay = 0;

    juce::SmoothedValue<float> mixSmoothed, gainSmoothed;
};

} // namespace birch
