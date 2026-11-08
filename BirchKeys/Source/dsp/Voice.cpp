#include "Voice.h"

namespace bk
{

namespace
{
    constexpr float organRatios[9] = { 0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f };
    constexpr float stopFadeSeconds = 0.012f;

    float octavesFrom (float note, float reference) { return (note - reference) / 12.0f; }

    // Rhodes pickup: the flux through the coil is a bell curve over the tine position (DAGA 2017).
    // x and the offset p are in units of the field width. Centred (p = 0) the output is the octave.
    inline float magneticFlux (float x) { return std::exp (-x * x); }

    float waveSample (Wave w, float t, float dt, float pulseWidth)
    {
        switch (w)
        {
            case Wave::sine:     return std::sin (twoPi * t);
            case Wave::triangle: return 4.0f * std::abs (t - 0.5f) - 1.0f;
            case Wave::saw:      return 2.0f * t - 1.0f - polyBlep (t, dt);
            case Wave::square:
            {
                // pulse = difference of two saws
                float t2 = t + pulseWidth;
                if (t2 >= 1.0f)
                    t2 -= 1.0f;
                const float a = 2.0f * t - 1.0f - polyBlep (t, dt);
                const float b = 2.0f * t2 - 1.0f - polyBlep (t2, dt);
                return a - b; // zero mean for any width
            }
        }
        return 0.0f;
    }
}

void Voice::prepare (float sampleRate)
{
    sr = sampleRate;
    int size = 1;
    while (size < (int) (sampleRate / 20.0f) + 16)
        size <<= 1;
    line.assign ((size_t) size, 0.0f);
    lineMask = size - 1;
    active = false;
}

void Voice::start (const InstrumentDef& d, int newKey, int note, float vel, float pan,
                   const VoiceControls& c, bool firstInPhrase)
{
    def = &d;
    key = newKey;
    velocity = std::clamp (vel, 0.02f, 1.0f);
    currentNote = targetNote = (float) note;
    active = true;
    released = false;
    sustained = false;
    ageSamples = 0;
    quietSamples = 0;
    fade = 1.0f;
    fadeStep = 0.0f;
    const float attackTime = std::max (c.attack, d.attack);
    const bool swells = attackTime > 0.0005f && d.engine != Engine::analog;
    swell = swells ? 0.0f : 1.0f;
    swellStep = swells ? 1.0f / (attackTime * sr) : 0.0f;
    vibPhase = 0.0f;
    rng.state = 0x9e3779b9u * (uint32_t) (note + 1) + order * 2654435761u;

    // equal-power pan
    const float p = std::clamp (pan, -1.0f, 1.0f);
    panL = std::cos ((p + 1.0f) * 0.25f * pi);
    panR = std::sin ((p + 1.0f) * 0.25f * pi);

    noiseEnv = 0.0f;
    noiseFilter.reset();

    // body colour and output low-pass, used by the modal, organ and fm engines
    numBody = 0;
    for (const auto& f : d.body)
        if (f.gain > 0.0f && f.hz > 0.0f)
        {
            bodyFilter[numBody].reset();
            bodyFilter[numBody].set (f.hz, f.q, sr);
            bodyGain[numBody++] = f.gain;
        }
    lpCoef = d.lowpass > 0.0f ? onePoleCoef (std::min (d.lowpass, 0.45f * sr), sr) : 0.0f;
    lpState = 0.0f;

    switch (d.engine)
    {
        case Engine::modal:  startModal (c); break;
        case Engine::string: startString (c); break;
        case Engine::organ:  startOrgan (c, firstInPhrase); break;
        case Engine::analog: startAnalog (c); break;
        case Engine::fm:     startFm (c); break;
    }
}

void Voice::legatoTo (int newKey, int note)
{
    key = newKey;
    targetNote = (float) note;
    released = false;
    sustained = false;
    mwahAge = 0.0f; // the fretless "mwah" blooms again on every new note
}

void Voice::release()
{
    if (! active || released)
        return;
    released = true;
    sustained = false;

    switch (def->engine)
    {
        case Engine::organ:
            for (int i = 0; i < 9; ++i)
            {
                orgTarget[i] = 0.0f;
                orgContact[i] = ageSamples + (int) (rng.unipolar() * 0.002f * sr);
            }
            break;
        case Engine::analog:
            ampEnv.noteOff();
            break;
        case Engine::string:
        {
            const float releaseT60 = def->releaseTau * 6.9f;
            if (releaseT60 < t60)
                loopGain = std::pow (10.0f, -3.0f / (releaseT60 * midiToHz (currentNote)));
            // Clavinet: the tangent lifting off knocks the string
            if (def->fingerNoise > 0.0f && def->pickupResHz > 4000.0f)
            {
                noiseEnv = def->fingerNoise * 0.5f;
                noiseCoef = decayCoef (0.004f, sr);
            }
            break;
        }
        case Engine::fm:
            for (int i = 0; i < def->numFm; ++i)
                fmOps[i].ampCoef = decayCoef (def->releaseTau, sr);
            break;
        case Engine::modal:
            modesPitch = -1.0f; // recompute with the damper on
            break;
    }
}

void Voice::quickStop()
{
    if (! active)
        return;
    fadeStep = 1.0f / (stopFadeSeconds * sr);
}

float Voice::nextGain()
{
    const float g = fade * swell;
    if (fadeStep > 0.0f)
        fade = std::max (0.0f, fade - fadeStep);
    if (swell < 1.0f)
        swell = std::min (1.0f, swell + swellStep);
    return g;
}

void Voice::finishBlock (float peak, int n)
{
    ageSamples += n;
    if (peak < 2.0e-5f && ageSamples > (int) (0.02f * sr) && swell >= 1.0f)
        quietSamples += n;
    else
        quietSamples = 0;

    if (quietSamples > (int) (0.05f * sr) || fade <= 0.0f)
        active = false;
}

float Voice::vibratoSemis (const VoiceControls& c, int n)
{
    if (def->motion != Motion::vibrato || c.vibrato <= 0.0f)
        return 0.0f;
    const float ramp = std::clamp ((float) ageSamples / (def->vibratoDelay * sr + 1.0f), 0.0f, 1.0f);
    vibPhase += twoPi * def->motionRate * (float) n / sr;
    if (vibPhase > twoPi)
        vibPhase -= twoPi;
    return 0.35f * c.vibrato * ramp * std::sin (vibPhase);
}

float Voice::colour (float x)
{
    float y = x;
    for (int i = 0; i < numBody; ++i)
        y += bodyGain[i] * bodyFilter[i].bandpass (x);
    if (lpCoef > 0.0f)
    {
        lpState += lpCoef * (y - lpState);
        y = lpState;
    }
    return y;
}

void Voice::render (float* left, float* right, int numSamples, const VoiceControls& c)
{
    if (! active)
        return;

    // process in short chunks so pitch, glide and vibrato stay smooth
    constexpr int chunk = 16;
    for (int offset = 0; offset < numSamples && active; offset += chunk)
    {
        const int n = std::min (chunk, numSamples - offset);
        switch (def->engine)
        {
            case Engine::modal:  renderModal (left + offset, right + offset, n, c); break;
            case Engine::string: renderString (left + offset, right + offset, n, c); break;
            case Engine::organ:  renderOrgan (left + offset, right + offset, n, c); break;
            case Engine::analog: renderAnalog (left + offset, right + offset, n, c); break;
            case Engine::fm:     renderFm (left + offset, right + offset, n, c); break;
        }
    }
}

//==============================================================================
// Modal: sums of exponentially decaying sinusoids, each a complex rotator
void Voice::startModal (const VoiceControls& c)
{
    const auto& d = *def;
    const float f0 = midiToHz (currentNote);
    const float maxFreq = std::min (0.45f * sr, 18000.0f);
    const float decayScale = c.decayMul * std::pow (d.decayKeytrack, octavesFrom (currentNote, 60.0f));
    const float tilt = (c.tone - 0.5f) * 1.4f;
    const bool hasPickup = d.pickup != Pickup::none;
    const float loud = std::pow (velocity, 1.3f);
    numModes = 0;

    auto add = [&] (float ratio, float amp, float tau)
    {
        if (f0 * ratio > maxFreq || numModes >= maxModes || amp <= 0.0f)
            return;
        auto& m = modes[numModes++];
        m.ratio = ratio;
        m.tau = tau;
        m.re = amp;
        m.im = 0.0f;
    };

    if (d.pianoPartials)
    {
        // stiff string: f_n = n f0 sqrt (1 + B n^2), B grows towards the treble
        const float B = std::clamp (d.inharmonicity * std::exp2 (0.9f * octavesFrom (currentNote, 60.0f)), 5.0e-5f, 0.02f);
        // felt hammer: contact time shrinks with velocity and in the treble, so the cut-off rises
        const float hammerCut = (300.0f + 3.0f * f0) * (0.6f + 2.4f * std::pow (velocity, 1.3f))
                                * d.hammerBright * std::exp2 ((c.tone - 0.5f) * 2.0f) * (0.5f + c.character);
        const float tau1 = d.baseDecay * decayScale;
        const float detune = std::exp2 (d.unisonCents / 1200.0f);
        const int partials = std::min (d.maxPartials, maxModes / 2);

        for (int n = 1; n <= partials; ++n)
        {
            const float ratio = (float) n * std::sqrt ((1.0f + B * (float) (n * n)) / (1.0f + B));
            const float fn = f0 * ratio;
            const float strike = std::abs (std::sin ((float) n * pi * d.hammerPos)) / std::pow ((float) n, 0.9f);
            const float hammer = 1.0f / (1.0f + std::pow (fn / hammerCut, 2.4f));
            const float amp = strike * hammer * loud;
            const float tau = tau1 / (1.0f + 0.12f * (float) (n - 1) + (fn / 2500.0f) * (fn / 2500.0f));
            // the unison strings: a fast "prompt" stage and a slow aftersound, slightly apart in pitch
            add (ratio / std::sqrt (detune), amp * d.promptShare, tau * d.promptRatio);
            add (ratio * std::sqrt (detune), amp * (1.0f - d.promptShare), tau);
        }
        noiseEnv = d.noiseAmount * std::pow (velocity, 1.5f) * (0.5f + c.character);
    }
    else
    {
        // hardness (CHARACTER) decides how much velocity it takes to excite the upper modes
        const float hardness = hasPickup ? 1.0f : 1.6f - 1.2f * c.character;
        for (int i = 0; i < d.numModes; ++i)
        {
            const auto& md = d.modes[i];
            float amp = md.amp * std::pow (velocity, md.velBright * 2.5f * hardness);
            if (md.ratio > 1.01f)
                amp *= std::pow (md.ratio, tilt);
            if (! hasPickup)
                amp *= loud;
            float tau = md.decay * decayScale;
            if (d.transientMs > 0.0f && md.ratio > 1.01f)
                tau = std::min (tau, d.transientMs * 0.001f); // upper modes live only in the transient
            add (md.ratio, amp, tau);
        }
        // Rhodes tonebar: its own eigenfrequency, far below the note, rings for the transient only
        if (d.tonebarRatio > 0.0f)
        {
            const float ratio = d.tonebarRatio * std::exp2 (-0.25f * octavesFrom (currentNote, 60.0f));
            add (ratio, 0.5f * velocity, d.transientMs * 0.0012f);
        }
        noiseEnv = d.noiseAmount * std::pow (velocity, 1.5f) * (hasPickup ? 1.0f : 0.5f + c.character);
    }

    noiseCoef = decayCoef (d.noiseDecay, sr);
    noiseFilter.set (std::min (d.noiseFreq * (0.5f + velocity), 0.45f * sr), 0.7f, sr);

    // how far the tine swings through the pickup field: harder hits and longer (lower) tines
    // deflect more, which is where the growl comes from
    deflectionScale = d.deflection * velocity * std::pow (261.6f / f0, 0.45f) * (0.4f + 1.2f * c.character);
    deflectionScale = std::clamp (deflectionScale, 0.02f, 1.6f);
    pickupOffset = d.pickupOffset * 1.3f;
    pickupNorm = 1.0f / (0.86f * twoPi * f0 / sr); // max slope of the bell curve times the fundamental's dx/dt
    pickupPrev = d.pickup == Pickup::magnetic ? magneticFlux (-pickupOffset) : 0.0f;
    hpState = hpPrev = 0.0f;
    hpCoef = std::exp (-twoPi * 150.0f / sr);

    outGain = d.gain * (hasPickup ? std::pow (velocity, 0.6f) : 1.0f);
    modesPitch = -1.0f;
}

void Voice::updateModes (float pitchMul)
{
    const bool damped = released && ! sustained;
    if (std::abs (pitchMul - modesPitch) < 1.0e-7f && damped == modesDamped)
        return;
    modesPitch = pitchMul;
    modesDamped = damped;

    const float f0 = midiToHz (currentNote) * pitchMul;
    for (int i = 0; i < numModes; ++i)
    {
        auto& m = modes[i];
        const float w = twoPi * f0 * m.ratio / sr;
        const float tau = damped ? std::min (m.tau, def->releaseTau) : m.tau;
        const float r = w < 0.95f * pi ? decayCoef (tau, sr) : 0.0f;
        m.cr = r * std::cos (w);
        m.ci = r * std::sin (w);
    }
}

void Voice::renderModal (float* left, float* right, int n, const VoiceControls& c)
{
    updateModes (std::exp2 (c.pitchBend / 12.0f));
    const Pickup pickup = def->pickup;
    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float x = 0.0f;
        for (int k = 0; k < numModes; ++k)
        {
            auto& m = modes[k];
            const float re = m.re * m.cr - m.im * m.ci;
            m.im = m.re * m.ci + m.im * m.cr;
            m.re = re;
            x += m.im;
        }

        float y = x;
        if (pickup == Pickup::magnetic)
        {
            // coil voltage = change of flux; the bell-shaped field makes it asymmetric
            const float pos = std::clamp (x * deflectionScale, -2.5f, 2.5f) - pickupOffset;
            const float flux = magneticFlux (pos);
            y = (flux - pickupPrev) * pickupNorm / deflectionScale;
            pickupPrev = flux;
        }
        else if (pickup == Pickup::electrostatic)
        {
            // reed/plate capacitor C = 1/(1 - x): current = dC/dt, asymmetric, then the RC high-pass
            const float xs = std::clamp (x * deflectionScale * 0.7f, -2.0f, 0.88f);
            const float cap = xs / (1.0f - xs);
            hpState = hpCoef * (hpState + cap - hpPrev);
            hpPrev = cap;
            y = fastTanh (1.2f * hpState) / (1.2f * deflectionScale * 0.7f);
        }

        if (noiseEnv > 1.0e-6f)
        {
            y += noiseFilter.lowpass (rng.bipolar()) * noiseEnv;
            noiseEnv *= noiseCoef;
        }

        y = colour (y) * outGain * nextGain();
        left[i] += y * panL;
        right[i] += y * panR;
        peak = std::max (peak, std::abs (y));
    }
    finishBlock (peak, n);
}

//==============================================================================
// String: digital waveguide (Karplus-Strong with Jaffe-Smith extensions)
void Voice::startString (const VoiceControls& c)
{
    const auto& d = *def;
    const float f0 = midiToHz (currentNote);
    const int size = lineMask + 1;

    baseDelay = std::clamp (sr / f0, 4.0f, (float) size * 0.7f);
    // one-pole loop loss grows with frequency squared, so scale it down towards the treble
    damping = std::clamp (d.stringDamping * std::sqrt (261.6f / f0) * std::exp2 (-(c.tone - 0.5f) * 1.5f), 0.01f, 0.93f);
    lossDelay = damping / (1.0f - damping); // phase delay of the loop low-pass at low frequencies
    t60 = d.t60 * c.decayMul * std::pow (d.decayKeytrack, octavesFrom (currentNote, 36.0f));
    {
        // the loop low-pass also attenuates the fundamental: compensate so T60 holds at every pitch
        const float w = twoPi * f0 / sr;
        const float re = 1.0f - damping * std::cos (w), im = damping * std::sin (w);
        const float lpMag = (1.0f - damping) / std::sqrt (re * re + im * im);
        loopGain = std::min (0.9999f, std::pow (10.0f, -3.0f / (t60 * f0)) / std::max (0.2f, lpMag));
    }
    loopLp = 0.0f;

    // excitation: the string pulled into a triangle at the pluck point. A wider finger (and a
    // softer hit) rounds the corner, which removes highs; hard hits add some noise.
    std::fill (line.begin(), line.end(), 0.0f);
    const int len = (int) baseDelay;
    const float hard = std::clamp (velocity * (0.4f + 1.2f * c.character), 0.0f, 1.0f);
    const float apex = std::clamp (d.pluckPos, 0.02f, 0.98f) * (float) len;
    const float width = std::max (1.0f, d.pluckWidth * (1.3f - 0.6f * hard) * (float) len);
    const float smoothCoef = std::clamp (2.0f / (width + 1.0f), 0.02f, 1.0f);
    float smooth = 0.0f;
    float mean = 0.0f;

    std::vector<float>& buf = line;
    for (int i = 0; i < len; ++i)
    {
        const float x = (float) i;
        float v = x < apex ? x / apex : ((float) len - x) / ((float) len - apex);
        v += d.pluckNoise * hard * rng.bipolar() * 0.5f;
        smooth += smoothCoef * (v - smooth);
        buf[(size_t) ((writePos - len + i) & lineMask)] = smooth;
        mean += smooth;
    }
    mean /= (float) len;
    float peak = 1.0e-6f;
    for (int i = 0; i < len; ++i)
    {
        auto& s = buf[(size_t) ((writePos - len + i) & lineMask)];
        s -= mean;
        peak = std::max (peak, std::abs (s));
    }
    const float scale = std::pow (velocity, 1.2f) / peak;
    for (int i = 0; i < len; ++i)
        buf[(size_t) ((writePos - len + i) & lineMask)] *= scale;

    dropEnv = 1.0f;
    dropCoef = decayCoef (0.07f, sr);

    pickupRes.reset();
    if (d.pickupResHz > 0.0f)
        pickupRes.set (std::min (d.pickupResHz, 0.45f * sr), d.pickupResQ, sr);
    postFilter.reset();
    postFilter.set (std::min (d.postLowpass * std::exp2 ((c.tone - 0.5f) * 2.0f), 0.45f * sr), 0.707f, sr);
    mwahFilter.reset();
    mwahAge = 0.0f;
    dc.reset();

    noiseEnv = d.fingerNoise * velocity * (0.5f + c.character);
    noiseCoef = decayCoef (0.015f, sr);
    noiseFilter.set (std::min (d.noiseFreq, 0.45f * sr), 0.8f, sr);

    outGain = d.gain;
}

float Voice::readLine (float delay) const
{
    float rp = (float) writePos - delay;
    const float size = (float) (lineMask + 1);
    while (rp < 0.0f)
        rp += size;
    const int i = (int) rp;
    const float f = rp - (float) i;
    const float x0 = line[(size_t) ((i - 1) & lineMask)];
    const float x1 = line[(size_t) (i & lineMask)];
    const float x2 = line[(size_t) ((i + 1) & lineMask)];
    const float x3 = line[(size_t) ((i + 2) & lineMask)];
    // 4-point Lagrange interpolation
    const float c1 = x2 - x0 / 3.0f - x1 * 0.5f - x3 / 6.0f;
    const float c2 = 0.5f * (x0 + x2) - x1;
    const float c3 = (x3 - x0) / 6.0f + 0.5f * (x1 - x2);
    return ((c3 * f + c2) * f + c1) * f + x1;
}

void Voice::renderString (float* left, float* right, int n, const VoiceControls& c)
{
    const auto& d = *def;
    const float pitchMul = std::exp2 ((c.pitchBend + vibratoSemis (c, n)) / 12.0f);
    const float bodyMix = d.bodyMix;
    const bool hasPickupRes = d.pickupResHz > 0.0f;
    float peak = 0.0f;

    // fretless "mwah": a formant that starts high and slides down over the first quarter second
    const bool mwah = d.mwah > 0.0f;
    if (mwah)
    {
        const float centre = 450.0f + 1000.0f * std::exp (-mwahAge / 0.25f);
        mwahFilter.set (centre, 3.0f, sr);
        mwahAge += (float) n / sr;
    }

    for (int i = 0; i < n; ++i)
    {
        // a plucked string starts a little sharp and settles as the tension relaxes
        const float delay = std::max (2.0f, baseDelay / (pitchMul * (1.0f + d.pitchDrop * velocity * dropEnv)) - lossDelay);
        dropEnv *= dropCoef;

        const float s = readLine (delay);
        loopLp = (1.0f - damping) * s + damping * loopLp;
        line[(size_t) writePos] = loopLp * loopGain;
        writePos = (writePos + 1) & lineMask;

        float out = s;
        if (d.pickupPos > 0.0f)
            out -= 0.9f * readLine (delay + d.pickupPos * baseDelay);
        if (hasPickupRes)
            out = pickupRes.lowpass (out);

        if (bodyMix > 0.0f)
        {
            float resonance = 0.0f;
            for (int k = 0; k < numBody; ++k)
                resonance += bodyFilter[k].bandpass (out) * bodyGain[k];
            out = out * (1.0f - bodyMix) + resonance * bodyMix * 2.0f;
        }

        if (mwah)
            out += d.mwah * 1.0f * mwahFilter.bandpass (out) * std::exp (-mwahAge / 0.6f);

        if (noiseEnv > 1.0e-6f)
        {
            out += noiseFilter.lowpass (rng.bipolar()) * noiseEnv;
            noiseEnv *= noiseCoef;
        }

        out = dc.process (postFilter.lowpass (out)) * outGain * nextGain();
        left[i] += out * panL;
        right[i] += out * panR;
        peak = std::max (peak, std::abs (out));
    }
    finishBlock (peak, n);
}

//==============================================================================
// Organ: nine generators per key (tonewheel sines, or transistor saw/square), contact clicks,
// single-trigger percussion, pipe "chiff"
void Voice::startOrgan (const VoiceControls& c, bool firstInPhrase)
{
    const auto& d = *def;
    const float f0 = midiToHz (currentNote);
    const bool tonewheel = d.organWave == Wave::sine && d.pipeDrift <= 0.0f;
    float total = 0.0f;

    for (int i = 0; i < 9; ++i)
    {
        float ratio = organRatios[i];
        while (tonewheel && f0 * ratio > 5900.0f) // the top tonewheels fold back an octave
            ratio *= 0.5f;
        orgRatio[i] = ratio;
        orgDrift[i] = d.pipeDrift > 0.0f ? std::exp2 (d.pipeDrift * rng.bipolar() / 1200.0f) : 1.0f;

        const float level = d.drawbars[i];
        orgTarget[i] = level > 0.0f ? std::pow (10.0f, -(8.0f - level) * 3.0f / 20.0f) : 0.0f; // ~3 dB per step
        total += orgTarget[i];
        orgGain[i] = 0.0f;

        const float phase = rng.unipolar(); // generators run free
        orgRe[i] = std::cos (twoPi * phase);
        orgIm[i] = std::sin (twoPi * phase);
        orgPhase[i] = phase;
        orgContact[i] = (int) (rng.unipolar() * 0.003f * sr);
    }
    orgNorm = 1.0f / (0.6f + 0.35f * total);

    percRatio = (float) d.percussionHarmonic;
    percEnv = firstInPhrase ? 1.4f * c.character : 0.0f;
    percCoef = decayCoef (d.percussionDecay * c.decayMul, sr);
    percRe = 1.0f;
    percIm = 0.0f;

    clickEnv = 0.0f;
    clickCoef = decayCoef (0.0015f, sr);
    clickFilter.reset();
    clickFilter.set (2500.0f, 0.7f, sr);

    // flue pipe speech: a burst of noise around the 2nd-3rd partial while the air column settles
    chiffEnv = d.chiff * (0.4f + 0.6f * velocity);
    chiffCoef = decayCoef (0.03f, sr);
    chiffFilter.reset();
    chiffFilter.set (std::min (f0 * 2.5f, 0.4f * sr), 2.0f, sr);

    orgAttackStep = 1.0f - std::exp (-1.0f / (std::max (0.0002f, d.organAttack) * sr));
    orgReleaseCoef = decayCoef (std::max (0.002f, d.releaseTau * c.decayMul), sr);

    modesPitch = -1.0f;
    outGain = d.gain;
}

void Voice::renderOrgan (float* left, float* right, int n, const VoiceControls& c)
{
    const auto& d = *def;
    const float pitchMul = std::exp2 ((c.pitchBend + vibratoSemis (c, n)) / 12.0f);
    if (std::abs (pitchMul - modesPitch) > 1.0e-7f)
    {
        modesPitch = pitchMul;
        const float f0 = midiToHz (currentNote) * pitchMul;
        for (int i = 0; i < 9; ++i)
        {
            const float f = f0 * orgRatio[i] * orgDrift[i];
            const float w = twoPi * f / sr;
            orgCr[i] = std::cos (w);
            orgCi[i] = std::sin (w);
            orgInc[i] = f / sr;
        }
        const float wp = twoPi * f0 * percRatio / sr;
        percCr = std::cos (wp);
        percCi = std::sin (wp);
    }

    const float waveMix = d.organWaveMix;
    const bool shaped = waveMix > 0.0f;
    float peak = 0.0f, energy = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const int age = ageSamples + i;
        float s = 0.0f;
        for (int k = 0; k < 9; ++k)
        {
            if (age == orgContact[k] && d.keyClick > 0.0f)
                clickEnv += d.keyClick * 0.12f;
            if (age >= orgContact[k])
            {
                if (released)
                    orgGain[k] *= orgReleaseCoef;
                else
                    orgGain[k] += orgAttackStep * (orgTarget[k] - orgGain[k]);
            }
            const float re = orgRe[k] * orgCr[k] - orgIm[k] * orgCi[k];
            orgIm[k] = orgRe[k] * orgCi[k] + orgIm[k] * orgCr[k];
            orgRe[k] = re;

            float v = orgIm[k];
            if (shaped && orgTarget[k] > 0.0f && orgInc[k] < 0.45f)
            {
                v += waveMix * (waveSample (d.organWave, orgPhase[k], orgInc[k], 0.5f) * 0.7f - v);
                orgPhase[k] += orgInc[k];
                if (orgPhase[k] >= 1.0f) orgPhase[k] -= 1.0f;
            }
            s += orgGain[k] * v;
            energy += orgGain[k];
        }

        if (percEnv > 1.0e-5f)
        {
            const float re = percRe * percCr - percIm * percCi;
            percIm = percRe * percCi + percIm * percCr;
            percRe = re;
            s += percEnv * percIm * 0.5f;
            percEnv *= percCoef;
        }

        if (clickEnv > 1.0e-5f)
        {
            s += clickFilter.highpass (rng.bipolar()) * clickEnv;
            clickEnv *= clickCoef;
        }
        if (chiffEnv > 1.0e-5f)
        {
            s += chiffFilter.bandpass (rng.bipolar()) * chiffEnv * 0.6f;
            chiffEnv *= chiffCoef;
        }

        float out = colour (s * orgNorm) * outGain * nextGain();
        left[i] += out * panL;
        right[i] += out * panR;
        peak = std::max (peak, std::abs (out));
    }

    // keep the rotators on the unit circle
    for (int k = 0; k < 9; ++k)
    {
        const float mag = 1.0f / std::sqrt (orgRe[k] * orgRe[k] + orgIm[k] * orgIm[k]);
        orgRe[k] *= mag;
        orgIm[k] *= mag;
    }
    {
        const float mag = 1.0f / std::sqrt (percRe * percRe + percIm * percIm);
        percRe *= mag;
        percIm *= mag;
    }

    if (released && energy < 1.0e-4f * (float) n)
        active = false;
    finishBlock (peak, n);
}

//==============================================================================
// Analog: polyBLEP oscillators into a saturating ladder filter
void Voice::startAnalog (const VoiceControls& c)
{
    const auto& d = *def;
    for (int u = 0; u < 3; ++u)
    {
        phase1[u] = rng.unipolar();
        phase2[u] = rng.unipolar();
    }
    subPhase = 0.0f;
    ampEnv.set (std::max (0.0005f, std::max (c.attack, d.attack)), d.decay * c.decayMul, d.sustain, d.release * c.decayMul, sr);
    ampEnv.value = 0.0f;
    ampEnv.noteOn();
    filterEnv = 1.0f;
    filterEnvCoef = decayCoef (d.filterDecay * c.decayMul, sr);
    pitchEnv = 1.0f;
    pitchEnvCoef = decayCoef (d.pitchEnvDecay, sr);
    breathChiff = d.breath > 0.0f ? 1.0f : 0.0f;
    breathChiffCoef = decayCoef (0.03f, sr);
    glideCoef = d.glide > 0.0f ? 1.0f - std::exp (-1.0f / (d.glide * sr)) : 1.0f;
    ladder[0].reset();
    ladder[1].reset();
    breathFilter.reset();
    formantFilter.reset();
    if (d.formant.gain > 0.0f)
        formantFilter.set (d.formant.hz, d.formant.q, sr);
    outGain = d.gain * (0.45f + 0.55f * velocity);
}

void Voice::renderAnalog (float* left, float* right, int n, const VoiceControls& c)
{
    const auto& d = *def;

    // glide towards the target note
    currentNote += (1.0f - std::pow (1.0f - glideCoef, (float) n)) * (targetNote - currentNote);

    // the 808's bridged-T slowly loses pitch as its capacitor leaks
    const float sag = d.resonantPitchHz > 0.0f ? -0.2f * (1.0f - std::exp (-(float) ageSamples / (0.6f * sr))) : 0.0f;
    const float note = currentNote + c.pitchBend + vibratoSemis (c, n) + d.pitchEnvSemis * pitchEnv + sag;
    const float f = midiToHz (note);
    const int unison = std::clamp (d.unison, 1, 3);
    float inc1[3], inc2[3];
    for (int u = 0; u < unison; ++u)
    {
        const float spread = unison > 1 ? ((float) u - 0.5f * (float) (unison - 1)) * d.unisonDetune : 0.0f;
        inc1[u] = f * std::exp2 (spread / 1200.0f) / sr;
        inc2[u] = f * std::exp2 ((d.osc2Semis * 100.0f + d.osc2Cents - spread * 0.7f) / 1200.0f) / sr;
    }
    const float subInc = 0.5f * f / sr;

    const float cutoff = d.cutoff * std::exp2 (d.keytrack * octavesFrom (currentNote, 60.0f))
                         * std::exp2 ((c.tone - 0.5f) * 4.0f) * std::exp2 (d.envAmount * filterEnv);
    ladder[0].set (cutoff, c.character, sr);
    ladder[1].set (cutoff, c.character, sr);
    if (d.breath > 0.0f)
        breathFilter.set (std::min (f, 0.4f * sr), 3.0f, sr);
    const float drive = d.drive * (1.0f + 0.5f * c.character);
    const bool hasFormant = d.formant.gain > 0.0f;

    const float unisonNorm = 1.0f / std::sqrt ((float) unison);
    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float l = 0.0f, r = 0.0f;
        for (int u = 0; u < unison; ++u)
        {
            float v = waveSample (d.osc1, phase1[u], inc1[u], d.pulseWidth);
            if (d.osc2Level > 0.0f)
                v += d.osc2Level * waveSample (d.osc2, phase2[u], inc2[u], d.pulseWidth);
            phase1[u] += inc1[u];
            if (phase1[u] >= 1.0f) phase1[u] -= 1.0f;
            phase2[u] += inc2[u];
            if (phase2[u] >= 1.0f) phase2[u] -= 1.0f;

            // unison voices spread left / centre / right
            const float pan = unison > 1 ? (float) u / (float) (unison - 1) : 0.5f;
            l += v * (1.0f - pan * 0.8f);
            r += v * (0.2f + pan * 0.8f);
        }
        l *= unisonNorm;
        r *= unisonNorm;

        float extra = 0.0f;
        if (d.subLevel > 0.0f)
        {
            extra += d.subLevel * waveSample (Wave::square, subPhase, subInc, 0.5f);
            subPhase += subInc;
            if (subPhase >= 1.0f) subPhase -= 1.0f;
        }
        if (d.noiseLevel > 0.0f)
            extra += d.noiseLevel * rng.bipolar();
        if (d.breath > 0.0f)
        {
            const float noise = rng.bipolar();
            extra += d.breath * (breathFilter.bandpass (noise) * 2.0f + breathChiff * noise * 0.6f);
            breathChiff *= breathChiffCoef;
        }
        l += extra;
        r += extra;

        const float a = ampEnv.process();
        float outL = ladder[0].process (fastTanh (l * drive * 0.5f)) * 2.0f;
        float outR = unison > 1 ? ladder[1].process (fastTanh (r * drive * 0.5f)) * 2.0f : outL;
        if (hasFormant)
        {
            // mute / bell: the formant replaces most of the direct sound
            const float mono = 0.5f * (outL + outR);
            const float fm = formantFilter.bandpass (mono) * d.formant.gain;
            outL = outL * 0.45f + fm;
            outR = outR * 0.45f + fm;
        }

        const float g = a * outGain * nextGain();
        left[i] += outL * g * panL;
        right[i] += outR * g * panR;
        peak = std::max (peak, std::abs (outL * g));
    }

    filterEnv *= std::pow (filterEnvCoef, (float) n);
    pitchEnv *= std::pow (pitchEnvCoef, (float) n);

    if (ampEnv.stage == Adsr::Stage::idle)
        active = false;
    finishBlock (peak, n);
}

//==============================================================================
// FM: three carrier/modulator pairs (DX7 algorithm 5 style)
void Voice::startFm (const VoiceControls& c)
{
    const auto& d = *def;
    const float decayScale = c.decayMul * std::pow (d.decayKeytrack, octavesFrom (currentNote, 60.0f));
    const float bright = (0.5f + c.character) * std::exp2 ((c.tone - 0.5f) * 1.5f);

    for (int i = 0; i < d.numFm; ++i)
    {
        const auto& p = d.fm[i];
        auto& s = fmOps[i];
        s.cPhase = rng.unipolar();
        s.mPhase = 0.0f;
        s.ampEnv = 1.0f;
        s.indexEnv = p.index * std::pow (velocity, p.velIndex * 1.5f) * bright;
        s.indexFloor = p.indexFloor * bright;
        s.indexCoef = decayCoef (p.indexDecay * std::sqrt (decayScale), sr);
        s.ampCoef = decayCoef (p.ampDecay * decayScale, sr);
        s.level = p.level;
        s.cInc = s.mInc = 0.0f;
    }
    fmNoiseEnv = d.fmNoise * velocity;
    fmNoiseCoef = decayCoef (0.006f, sr);
    noiseFilter.set (1200.0f, 0.7f, sr);
    glideCoef = d.glide > 0.0f ? 1.0f - std::exp (-1.0f / (d.glide * sr)) : 1.0f;
    outGain = d.gain * (0.3f + 0.7f * velocity);
}

void Voice::renderFm (float* left, float* right, int n, const VoiceControls& c)
{
    const auto& d = *def;
    currentNote += (1.0f - std::pow (1.0f - glideCoef, (float) n)) * (targetNote - currentNote);
    const float f0 = midiToHz (currentNote + c.pitchBend);
    for (int i = 0; i < d.numFm; ++i)
    {
        const auto& p = d.fm[i];
        const float detune = std::exp2 (p.detuneCents / 1200.0f);
        fmOps[i].cInc = f0 * p.carrierRatio * detune / sr;
        fmOps[i].mInc = f0 * p.modRatio * detune / sr;
    }

    float peak = 0.0f, energy = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.0f;
        for (int k = 0; k < d.numFm; ++k)
        {
            auto& op = fmOps[k];
            const float mod = std::sin (twoPi * op.mPhase) * op.indexEnv;
            s += op.level * op.ampEnv * std::sin (twoPi * op.cPhase + mod);
            op.cPhase += op.cInc;
            if (op.cPhase >= 1.0f) op.cPhase -= 1.0f;
            op.mPhase += op.mInc;
            if (op.mPhase >= 1.0f) op.mPhase -= 1.0f;
            op.indexEnv = op.indexFloor + (op.indexEnv - op.indexFloor) * op.indexCoef;
            op.ampEnv *= op.ampCoef;
            energy += op.ampEnv;
        }
        if (fmNoiseEnv > 1.0e-6f)
        {
            s += noiseFilter.lowpass (rng.bipolar()) * fmNoiseEnv;
            fmNoiseEnv *= fmNoiseCoef;
        }

        const float out = colour (s * 0.5f) * outGain * nextGain();
        left[i] += out * panL;
        right[i] += out * panR;
        peak = std::max (peak, std::abs (out));
    }

    if (energy < 1.0e-4f * (float) n)
        active = false;
    finishBlock (peak, n);
}

} // namespace bk
