#include "BirchEngine.h"

namespace birch
{

namespace
{
    constexpr float thumpCrossoverHz = 110.0f;
    constexpr float monoBassHz = 120.0f;
    constexpr double baseDelaySeconds = 0.004;
    constexpr double pitchWindowSeconds = 0.045;

    // Transparent below 0.8, rounds off smoothly to a ceiling of 1.0
    float softClip (float x)
    {
        constexpr float knee = 0.8f;
        const float a = std::abs (x);
        if (a <= knee)
            return x;
        const float y = knee + (1.0f - knee) * std::tanh ((a - knee) / (1.0f - knee));
        return std::copysign (y, x);
    }

    float gainToDb (float g) { return 20.0f * std::log10 (g + 1.0e-6f); }

    float onePoleCoef (double sr, double hz) { return 1.0f - (float) std::exp (-juce::MathConstants<double>::twoPi * hz / sr); }
}

void Engine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    sr = sampleRate;
    const juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlockSize, (juce::uint32) numChannels };
    const juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) maxBlockSize, 2 };

    lowCut.prepare (spec);
    lowCut.setType (juce::dsp::StateVariableTPTFilterType::highpass);

    pitchWindow = (int) std::round (pitchWindowSeconds * sampleRate);
    pitchDelay.prepare (spec);
    pitchDelay.setMaximumDelayInSamples (pitchWindow + 8);

    for (auto* x : { &thumpXover, &dryXover })
    {
        x->prepare (spec);
        x->setCutoffFrequency (thumpCrossoverHz);
    }

    // channel 0 = mid, 1 = side for the wet path; plain L/R for the dry path
    msXover.prepare (stereoSpec);
    msXover.setCutoffFrequency (monoBassHz);
    dryMsXover.prepare (spec);
    dryMsXover.setCutoffFrequency (monoBassHz);

    for (auto* f : { &preFilter, &toneFilter })
    {
        f->prepare (spec);
        f->setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    baseDelay = (int) std::round (baseDelaySeconds * sampleRate);
    for (auto* d : { &wobbleDelay, &dryDelay })
    {
        d->prepare (spec);
        d->setMaximumDelayInSamples (baseDelay * 2 + 16);
    }

    fastEnv.setTimes (sampleRate, 0.5f, 60.0f);
    slowEnv.setTimes (sampleRate, 20.0f, 60.0f);
    compEnv.setTimes (sampleRate, 8.0f, 90.0f);
    duckEnv.setTimes (sampleRate, 1.0f, 220.0f);
    crackleDecay = (float) std::exp (-1.0 / (0.0004 * sampleRate));

    for (auto* s : { &mixSmoothed, &gainSmoothed, &widthSmoothed })
        s->reset (sampleRate, 0.05);
    pitchBlend.reset (sampleRate, 0.03);

    reset();
}

void Engine::reset()
{
    lowCut.reset();
    pitchDelay.reset();
    thumpXover.reset();
    dryXover.reset();
    msXover.reset();
    dryMsXover.reset();
    preFilter.reset();
    toneFilter.reset();
    wobbleDelay.reset();
    dryDelay.reset();
    fastEnv.env = slowEnv.env = compEnv.env = duckEnv.env = 0;
    holdPhase = wowPhase = flutterPhase = pitchPhase = 0;
    drift = driftTarget = 0;
    for (int ch = 0; ch < 2; ++ch)
        held[ch] = hissLp[ch] = crackleEnv[ch] = crackleHpPrev[ch] = crackleHpOut[ch]
            = tapeLp[ch] = dcIn[ch] = dcOut[ch] = 0;
}

float Engine::noiseSample (int ch, float hissLevel, float crackleLevel, float crackleProb)
{
    const float white = rng.nextFloat() * 2.0f - 1.0f;

    hissLp[ch] += 0.35f * (white - hissLp[ch]);
    const float hiss = hissLp[ch] * hissLevel;

    if (rng.nextFloat() < crackleProb)
    {
        const float r = rng.nextFloat();
        float amp = 0.2f + 0.8f * r * r;
        if (rng.nextFloat() < 0.08f)
            amp *= 2.5f; // occasional bigger pop
        crackleEnv[ch] = rng.nextBool() ? amp : -amp;
    }

    const float raw = crackleEnv[ch] * (0.6f + 0.4f * white);
    crackleEnv[ch] *= crackleDecay;

    // one-pole high-pass so clicks don't thump
    crackleHpOut[ch] = 0.95f * (crackleHpOut[ch] + raw - crackleHpPrev[ch]);
    crackleHpPrev[ch] = raw;

    return hiss + crackleHpOut[ch] * crackleLevel;
}

void Engine::process (juce::AudioBuffer<float>& buffer, const Params& p, const juce::AudioBuffer<float>* sidechain)
{
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    const auto& era = eras[juce::jlimit (0, 2, p.era)];
    const float maxCut = (float) (sr * 0.45);

    const bool useSidechain = p.sidechain && sidechain != nullptr && sidechain->getNumChannels() > 0
                              && sidechain->getNumSamples() >= n;
    const int scChannels = useSidechain ? juce::jmin (sidechain->getNumChannels(), 2) : 0;

    lowCut.setCutoffFrequency (juce::jlimit (10.0f, 500.0f, p.lowCutHz));

    // Pitch
    const double pitchRatio = std::pow (2.0, p.pitch / 12.0);
    const double pitchInc = (1.0 - pitchRatio) / pitchWindow;
    pitchBlend.setTargetValue (std::abs (p.pitch) > 0.01f ? 1.0f : 0.0f);

    // Thump
    const float drive = 1.0f + 5.0f * p.thump;
    const float lowGain = (1.0f + 1.2f * p.thump) / drive;

    // Knock
    const bool knockOn = p.knock > 0.001f;
    const float transientAmt = 0.7f * p.knock;
    const float thrDb = -6.0f - 20.0f * p.knock;
    const float slope = 1.0f - 1.0f / (1.0f + 5.0f * p.knock);
    const float makeupDb = 7.0f * p.knock;
    gainReductionDb = 0;

    // Grit
    const float g = p.grit;
    const double targetRate = juce::jmin (sr, sr * std::pow ((double) era.minRate / sr, (double) g));
    const double holdInc = targetRate / sr;
    const float bits = 16.0f - (16.0f - era.minBits) * g;
    const float quant = std::pow (2.0f, bits - 1.0f);
    preFilter.setCutoffFrequency (juce::jlimit (1000.0f, maxCut, (float) targetRate * 0.5f * era.preFilter));
    toneFilter.setCutoffFrequency (juce::jlimit (1000.0f, maxCut, 20000.0f * std::pow (era.toneHz / 20000.0f, g)));

    // Warmth: small-signal gain stays at unity, peaks get rounded asymmetrically
    const bool warmthOn = p.warmth > 0.001f;
    const float warmDrive = 1.0f + 4.0f * p.warmth;
    const float warmBias = 0.2f * p.warmth;
    const float tanhBias = std::tanh (warmBias);
    const float warmNorm = 1.0f / (warmDrive * (1.0f - tanhBias * tanhBias));
    const float warmBlend = juce::jmin (1.0f, p.warmth * 4.0f);
    const float tapeCoef = onePoleCoef (sr, juce::jmin (maxCut, 20000.0f * std::pow (9000.0f / 20000.0f, p.warmth)));

    // Wobble
    const float w = p.wobble;
    const double wowInc = juce::MathConstants<double>::twoPi * 0.55 / sr;
    const double flutterInc = juce::MathConstants<double>::twoPi * 7.3 / sr;
    const float driftCoef = onePoleCoef (sr, 1.5);
    const float driftChangeProb = (float) (2.0 / sr);

    // Dust
    const bool dustOn = p.dust > 0.001f;
    const float hissLevel = p.dust * p.dust * 0.02f * era.hiss;
    const float crackleLevel = p.dust * 0.25f * era.crackle;
    const float crackleProb = (float) ((1.5 + 30.0 * p.dust) * era.crackle / sr);

    mixSmoothed.setTargetValue (p.bypass ? 0.0f : p.mix);
    gainSmoothed.setTargetValue (p.bypass ? 1.0f : juce::Decibels::decibelsToGain (p.outDb));
    widthSmoothed.setTargetValue (p.width);

    for (int i = 0; i < n; ++i)
    {
        float in[2], dry[2], wet[2];
        for (int ch = 0; ch < 2; ++ch)
            in[ch] = buffer.getSample (juce::jmin (ch, numCh - 1), i);

        const float inLevel = juce::jmax (std::abs (in[0]), std::abs (in[1]));
        float scLevel = 0.0f;
        for (int ch = 0; ch < scChannels; ++ch)
            scLevel = juce::jmax (scLevel, std::abs (sidechain->getSample (ch, i)));

        const float duck = 1.0f / (1.0f + 12.0f * duckEnv.process (useSidechain ? scLevel : inLevel));

        // Dry path: same all-passes and base delay as the wet path, so Mix blends cleanly
        for (int ch = 0; ch < numCh; ++ch)
        {
            float lo, hi;
            dryXover.processSample (ch, in[ch], lo, hi);
            dryMsXover.processSample (ch, lo + hi, lo, hi);
            dryDelay.pushSample (ch, lo + hi);
            dry[ch] = dryDelay.popSample (ch, (float) baseDelay);
        }

        // Low cut + pitch
        const float blend = pitchBlend.getNextValue();
        pitchPhase += pitchInc;
        pitchPhase -= std::floor (pitchPhase);
        const double phase2 = pitchPhase + 0.5 - std::floor (pitchPhase + 0.5);
        const float tapA = 1.0f + (float) (pitchPhase * pitchWindow);
        const float tapB = 1.0f + (float) (phase2 * pitchWindow);
        const float s = (float) std::sin (juce::MathConstants<double>::pi * pitchPhase);
        const float gainA = s * s;

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float x = lowCut.processSample (ch, in[ch]);
            pitchDelay.pushSample (ch, x);
            const float a = pitchDelay.popSample (ch, tapA, false);
            const float b = pitchDelay.popSample (ch, tapB, true);
            wet[ch] = x + blend * (gainA * a + (1.0f - gainA) * b - x);
        }

        // Thump
        for (int ch = 0; ch < numCh; ++ch)
        {
            float lo, hi;
            thumpXover.processSample (ch, wet[ch], lo, hi);
            wet[ch] = std::tanh (lo * drive) * lowGain + hi;
        }

        // Knock (stereo-linked; the compressor can listen to the sidechain instead)
        float knockGain = 1.0f;
        {
            const float det = numCh > 1 ? juce::jmax (std::abs (wet[0]), std::abs (wet[1])) : std::abs (wet[0]);
            const float fast = fastEnv.process (det);
            const float slow = slowEnv.process (det);
            const float level = compEnv.process (useSidechain ? scLevel : det);
            if (knockOn)
            {
                const float transientDb = juce::jlimit (0.0f, 18.0f, gainToDb ((fast + 1.0e-5f) / (slow + 1.0e-5f))) * transientAmt;
                const float reductionDb = juce::jmax (0.0f, gainToDb (level) - thrDb) * slope;
                gainReductionDb = juce::jmax (gainReductionDb, reductionDb);
                knockGain = juce::Decibels::decibelsToGain (transientDb - reductionDb + makeupDb);
            }
        }

        // Grit
        holdPhase += holdInc;
        const bool latch = holdPhase >= 1.0;
        if (latch)
            holdPhase -= 1.0;

        // Wobble modulation (shared by both channels, like a tape transport)
        float delaySamples = (float) baseDelay;
        if (w > 0.0f)
        {
            wowPhase += wowInc;
            flutterPhase += flutterInc;
            if (wowPhase > juce::MathConstants<double>::twoPi) wowPhase -= juce::MathConstants<double>::twoPi;
            if (flutterPhase > juce::MathConstants<double>::twoPi) flutterPhase -= juce::MathConstants<double>::twoPi;
            if (rng.nextFloat() < driftChangeProb)
                driftTarget = rng.nextFloat() * 2.0f - 1.0f;
            drift += driftCoef * (driftTarget - drift);

            const double mod = 0.0022 * std::sin (wowPhase) + 0.00012 * std::sin (flutterPhase) + 0.0008 * drift;
            delaySamples += (float) (w * mod * sr);
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = preFilter.processSample (ch, wet[ch] * knockGain);
            if (latch)
                held[ch] = std::round (x * quant) / quant;
            x = toneFilter.processSample (ch, held[ch]);

            if (warmthOn)
            {
                float t = (std::tanh (warmDrive * x + warmBias) - tanhBias) * warmNorm;
                tapeLp[ch] += tapeCoef * (t - tapeLp[ch]);
                t = tapeLp[ch] - dcIn[ch] + 0.9995f * dcOut[ch];
                dcIn[ch] = tapeLp[ch];
                dcOut[ch] = t;
                x += warmBlend * (t - x);
            }

            wobbleDelay.pushSample (ch, x);
            wet[ch] = wobbleDelay.popSample (ch, delaySamples);
        }

        // Width + mono bass
        const float width = widthSmoothed.getNextValue();
        if (numCh == 2)
        {
            float mLo, mHi, sLo, sHi;
            msXover.processSample (0, 0.5f * (wet[0] + wet[1]), mLo, mHi);
            msXover.processSample (1, 0.5f * (wet[0] - wet[1]), sLo, sHi);
            const float mid = mLo + mHi;
            const float side = (p.monoBass ? sHi : sLo + sHi) * width;
            wet[0] = mid + side;
            wet[1] = mid - side;
        }

        const float mix = mixSmoothed.getNextValue();
        const float gain = gainSmoothed.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = wet[ch];
            if (dustOn)
                x += noiseSample (ch, hissLevel, crackleLevel, crackleProb) * duck;

            const float out = (dry[ch] + mix * (x - dry[ch])) * gain;
            buffer.setSample (ch, i, softClip (out));
        }
    }
}

} // namespace birch
