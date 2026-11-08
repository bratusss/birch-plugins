#include "Effects.h"

namespace bk
{

//==============================================================================
void Leslie::prepare (float sampleRate)
{
    sr = sampleRate;
    crossover.set (800.0f, 0.707f, sr);
    hornDelay.prepare ((int) (0.004f * sr));
    drumDelay.prepare ((int) (0.006f * sr));
}

void Leslie::process (float* left, float* right, int n, float motion)
{
    // off below 4 %, chorale (slow) up to 55 %, tremolo (fast) above; speeds from RESEARCH.md
    const bool on = motion > 0.04f;
    const bool fast = motion > 0.55f;
    const float hornTarget = fast ? 6.8f : 0.83f;
    const float drumTarget = fast ? 5.8f : 0.67f;
    // the light horn spins up quickly, the heavy drum takes seconds
    const float hornInertia = onePoleCoef (1.0f / (fast ? 0.7f : 1.0f), sr);
    const float drumInertia = onePoleCoef (1.0f / (fast ? 3.5f : 4.5f), sr);
    const float wetStep = 1.0f / (0.05f * sr);

    const float hornBase = 0.0012f * sr, hornDepth = 0.0009f * sr;
    const float drumBase = 0.0025f * sr, drumDepth = 0.0012f * sr;

    for (int i = 0; i < n; ++i)
    {
        wet = on ? std::min (1.0f, wet + wetStep) : std::max (0.0f, wet - wetStep);
        hornSpeed += hornInertia * (hornTarget - hornSpeed);
        drumSpeed += drumInertia * (drumTarget - drumSpeed);
        hornAngle += twoPi * hornSpeed / sr;
        drumAngle += twoPi * drumSpeed / sr;
        if (hornAngle > twoPi) hornAngle -= twoPi;
        if (drumAngle > twoPi) drumAngle -= twoPi;

        const float mono = 0.5f * (left[i] + right[i]);
        float lo, band, hi;
        crossover.process (mono, lo, band, hi);
        hi += band * 0.5f;
        lo += band * 0.5f;
        hornDelay.push (hi);
        drumDelay.push (lo);

        // two microphones 90 degrees apart
        const float hs = std::sin (hornAngle), hc = std::cos (hornAngle);
        const float ds = std::sin (drumAngle), dcos = std::cos (drumAngle);
        const float hornL = hornDelay.read (hornBase + hornDepth * hs) * (1.0f + 0.45f * hc);
        const float hornR = hornDelay.read (hornBase + hornDepth * hc) * (1.0f - 0.45f * hs);
        const float drumL = drumDelay.read (drumBase + drumDepth * ds) * (1.0f + 0.25f * dcos);
        const float drumR = drumDelay.read (drumBase + drumDepth * dcos) * (1.0f - 0.25f * ds);

        left[i] += wet * (0.8f * (hornL + drumL) - left[i]);
        right[i] += wet * (0.8f * (hornR + drumR) - right[i]);
    }
}

//==============================================================================
void Room::prepare (float sampleRate)
{
    sr = sampleRate;
    const float ms[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
    const float rt60 = 0.9f;
    for (int i = 0; i < 4; ++i)
    {
        lengths[i] = ms[i] * 0.001f * sr;
        lines[i].prepare ((int) lengths[i] + 4);
        feedback[i] = std::pow (10.0f, -3.0f * (ms[i] * 0.001f) / rt60);
        lp[i] = 0.0f;
    }
}

void Room::process (float* left, float* right, int n, float mix)
{
    if (mix <= 0.001f)
        return;

    const float wet = mix * 0.55f;
    for (int i = 0; i < n; ++i)
    {
        float d[4];
        for (int k = 0; k < 4; ++k)
        {
            const float v = lines[k].read (lengths[k] - 1.0f);
            lp[k] += (1.0f - damp) * (v - lp[k]); // walls absorb the highs
            d[k] = lp[k];
        }

        // Householder feedback matrix keeps the network lossless before the feedback gains
        const float sum = 0.5f * (d[0] + d[1] + d[2] + d[3]);
        const float in = 0.5f * (left[i] + right[i]);
        const float signs[4] = { 1.0f, -1.0f, 1.0f, -1.0f };
        for (int k = 0; k < 4; ++k)
            lines[k].push ((d[k] - sum) * feedback[k] + in * signs[k] * 0.35f);

        left[i] += wet * (d[0] + d[2]);
        right[i] += wet * (d[1] + d[3]);
    }
}

//==============================================================================
namespace
{
    struct Era { float rate, bits, toneHz, preFilter; };
    // '88: extra dirty; '94: E-mu SP-1200 (26.04 kHz, 12-bit); '98: cleaner 40 kHz 12-bit
    constexpr Era eras[] = {
        { 12000.0f,  8.0f,  7000.0f, 1.3f },
        { 26040.0f, 12.0f, 12000.0f, 1.0f },
        { 40000.0f, 12.0f, 16000.0f, 0.9f },
    };
}

void KeysFx::prepare (float sampleRate)
{
    sr = sampleRate;
    room.prepare (sr);
    for (auto& d : wobbleDelay)
        d.prepare ((int) (0.01f * sr));
    for (int ch = 0; ch < 2; ++ch)
    {
        preFilter[ch].reset();
        toneFilter[ch].reset();
        spFilter[ch].reset();
        held[ch] = tapeLp[ch] = dcIn[ch] = dcOut[ch] = 0.0f;
        hissLp[ch] = crackleEnv[ch] = crackleHpPrev[ch] = crackleHpOut[ch] = 0.0f;
    }
    filterWet = 0.0f;
}

void KeysFx::process (float* left, float* right, int n, const FxParams& p)
{
    float* ch[2] = { left, right };

    room.process (left, right, n, p.room);

    // Wobble: shared wow & flutter, like a record or tape transport
    if (p.wobble > 0.001f)
    {
        const float wowInc = twoPi * 0.55f / sr, flutterInc = twoPi * 7.3f / sr;
        const float driftCoef = onePoleCoef (1.5f, sr);
        const float base = 0.004f * sr;
        for (int i = 0; i < n; ++i)
        {
            wowPhase += wowInc;
            flutterPhase += flutterInc;
            if (wowPhase > twoPi) wowPhase -= twoPi;
            if (flutterPhase > twoPi) flutterPhase -= twoPi;
            if (rng.unipolar() < 2.0f / sr)
                driftTarget = rng.bipolar();
            drift += driftCoef * (driftTarget - drift);
            const float delay = base + p.wobble * sr * (0.0022f * std::sin (wowPhase) + 0.00012f * std::sin (flutterPhase) + 0.0008f * drift);
            for (int c = 0; c < 2; ++c)
            {
                wobbleDelay[c].push (ch[c][i]);
                ch[c][i] = wobbleDelay[c].read (delay);
            }
        }
    }

    // Warmth: asymmetric tape saturation, head-loss low-pass, DC blocker
    if (p.warmth > 0.001f)
    {
        const float drive = 1.0f + 4.0f * p.warmth;
        const float bias = 0.2f * p.warmth;
        const float tanhBias = std::tanh (bias);
        const float norm = 1.0f / (drive * (1.0f - tanhBias * tanhBias));
        const float blend = std::min (1.0f, p.warmth * 4.0f);
        const float tapeCoef = onePoleCoef (std::min (0.45f * sr, 20000.0f * std::pow (9000.0f / 20000.0f, p.warmth)), sr);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                float t = (std::tanh (drive * x + bias) - tanhBias) * norm;
                tapeLp[c] += tapeCoef * (t - tapeLp[c]);
                t = tapeLp[c] - dcIn[c] + 0.9995f * dcOut[c];
                dcIn[c] = tapeLp[c];
                dcOut[c] = t;
                ch[c][i] = x + blend * (t - x);
            }
    }

    // Dust: vinyl hiss and crackle
    if (p.dust > 0.001f)
    {
        const float hiss = p.dust * p.dust * 0.015f;
        const float crackle = p.dust * 0.22f;
        const float prob = (1.5f + 30.0f * p.dust) / sr;
        const float decay = std::exp (-1.0f / (0.0004f * sr));
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float white = rng.bipolar();
                hissLp[c] += 0.35f * (white - hissLp[c]);
                if (rng.unipolar() < prob)
                {
                    const float r = rng.unipolar();
                    float amp = 0.2f + 0.8f * r * r;
                    if (rng.unipolar() < 0.08f)
                        amp *= 2.5f;
                    crackleEnv[c] = rng.bipolar() > 0.0f ? amp : -amp;
                }
                const float raw = crackleEnv[c] * (0.6f + 0.4f * white);
                crackleEnv[c] *= decay;
                crackleHpOut[c] = 0.95f * (crackleHpOut[c] + raw - crackleHpPrev[c]);
                crackleHpPrev[c] = raw;
                ch[c][i] += hissLp[c] * hiss + crackleHpOut[c] * crackle;
            }
    }

    // Grit: sample-and-hold to a lower rate, fewer bits, between anti-alias and output filters
    if (p.grit > 0.001f)
    {
        const auto& era = eras[std::clamp (p.era, 0, 2)];
        const float g = p.grit;
        const double rate = std::min ((double) sr, (double) sr * std::pow ((double) era.rate / sr, (double) g));
        const double inc = rate / sr;
        const float bits = 16.0f - (16.0f - era.bits) * g;
        const float quant = std::pow (2.0f, bits - 1.0f);
        const float maxCut = 0.45f * sr;
        for (int c = 0; c < 2; ++c)
        {
            preFilter[c].set (std::clamp ((float) rate * 0.5f * era.preFilter, 1000.0f, maxCut), 0.707f, sr);
            toneFilter[c].set (std::clamp (20000.0f * std::pow (era.toneHz / 20000.0f, g), 1000.0f, maxCut), 0.707f, sr);
        }
        for (int i = 0; i < n; ++i)
        {
            holdPhase += inc;
            const bool latch = holdPhase >= 1.0;
            if (latch)
                holdPhase -= 1.0;
            for (int c = 0; c < 2; ++c)
            {
                const float x = preFilter[c].lowpass (ch[c][i]);
                if (latch)
                    held[c] = std::round (x * quant) / quant;
                ch[c][i] = toneFilter[c].lowpass (held[c]);
            }
        }
    }

    // SP filter: 4-pole low-pass with resonance, crossfaded out when fully open
    const float targetWet = p.filter < 0.995f ? 1.0f : 0.0f;
    if (targetWet > 0.0f || filterWet > 0.0f)
    {
        const float cutoff = 60.0f * std::pow (20000.0f / 60.0f, p.filter);
        for (auto& f : spFilter)
            f.set (cutoff, p.resonance * 0.9f, sr);
        const float step = 1.0f / (0.02f * sr);
        for (int i = 0; i < n; ++i)
        {
            filterWet = targetWet > filterWet ? std::min (1.0f, filterWet + step) : std::max (0.0f, filterWet - step);
            for (int c = 0; c < 2; ++c)
            {
                const float x = ch[c][i];
                const float y = spFilter[c].process (x * 0.7f) / 0.7f;
                ch[c][i] = x + filterWet * (y - x);
            }
        }
    }

    // Volume + safety clip
    const float target = std::pow (10.0f, p.volumeDb / 20.0f);
    const float step = (target - volume) / (float) std::max (1, n);
    for (int i = 0; i < n; ++i)
    {
        volume += step;
        left[i] = softClip (left[i] * volume);
        right[i] = softClip (right[i] * volume);
    }
}

} // namespace bk
