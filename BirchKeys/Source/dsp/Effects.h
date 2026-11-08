#pragma once

#include "Common.h"
#include <vector>

namespace bk
{

// Simple circular delay with linear interpolation, for modulation effects
struct ModDelay
{
    void prepare (int maxSamples)
    {
        int size = 1;
        while (size < maxSamples + 4)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        pos = 0;
    }

    void push (float x)
    {
        buffer[(size_t) pos] = x;
        pos = (pos + 1) & mask;
    }

    // delay in samples behind the most recent push
    float read (float delay) const
    {
        float rp = (float) pos - 1.0f - delay;
        while (rp < 0.0f)
            rp += (float) (mask + 1);
        const int i = (int) rp;
        const float f = rp - (float) i;
        return buffer[(size_t) (i & mask)] * (1.0f - f) + buffer[(size_t) ((i + 1) & mask)] * f;
    }

    std::vector<float> buffer;
    int mask = 0, pos = 0;
};

// Rotating speaker: horn above 800 Hz, drum below, each with Doppler delay, AM and inertia
class Leslie
{
public:
    void prepare (float sampleRate);
    void process (float* left, float* right, int n, float motion);

private:
    float sr = 44100.0f;
    Svf crossover;
    ModDelay hornDelay, drumDelay;
    float hornAngle = 0, drumAngle = 0, hornSpeed = 0, drumSpeed = 0, wet = 0;
};

// Small room: 4-line feedback delay network with damping
class Room
{
public:
    void prepare (float sampleRate);
    void process (float* left, float* right, int n, float mix);

private:
    float sr = 44100.0f;
    ModDelay lines[4];
    float lengths[4] {}, feedback[4] {}, lp[4] {};
    float damp = 0.3f;
};

struct FxParams
{
    float grit = 0;      // 0..1
    int era = 1;         // 0 = '88, 1 = '94 (SP-1200), 2 = '98
    float filter = 1;    // 1 = open
    float resonance = 0;
    float warmth = 0, wobble = 0, dust = 0, room = 0;
    float volumeDb = 0;
};

// The sampler chain from RESEARCH.md: room -> vinyl/tape -> sampler -> SP filter
class KeysFx
{
public:
    void prepare (float sampleRate);
    void process (float* left, float* right, int n, const FxParams&);

private:
    float sr = 44100.0f;
    Room room;

    ModDelay wobbleDelay[2];
    float wowPhase = 0, flutterPhase = 0, drift = 0, driftTarget = 0;

    float tapeLp[2] {}, dcIn[2] {}, dcOut[2] {};

    Rng rng { 777 };
    float hissLp[2] {}, crackleEnv[2] {}, crackleHpPrev[2] {}, crackleHpOut[2] {};

    Svf preFilter[2], toneFilter[2];
    double holdPhase = 0;
    float held[2] {};

    Ladder spFilter[2];
    float filterWet = 0, volume = 1;
};

} // namespace bk
