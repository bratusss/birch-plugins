#include "Part.h"

namespace bk
{

PartSettings defaultsFor (int index)
{
    const auto& list = instruments();
    const auto& d = list[(size_t) std::clamp (index, 0, (int) list.size() - 1)];
    PartSettings s;
    s.instrument = index;
    s.character = d.characterDefault;
    s.motion = d.motionDefault;
    s.rate = d.motionRate;
    s.width = d.spread;
    s.attack = d.attack;
    s.glide = d.glide;
    s.mono = d.mono;
    s.detune = d.unisonCents;
    s.pickupOffset = d.pickupOffset;
    s.pluckPos = d.pluckPos;
    s.body = d.bodyMix;
    s.finger = d.fingerNoise;
    s.pickupPos = d.pickupPos;
    s.pitchDrop = d.pitchDrop;
    s.mwah = d.mwah;
    std::copy (std::begin (d.drawbars), std::end (d.drawbars), s.drawbars);
    s.percHarmonic = d.percussionHarmonic;
    s.percDecay = d.percussionDecay;
    s.click = d.keyClick;
    s.chiff = d.chiff;
    s.osc1 = (int) d.osc1;
    s.osc2 = (int) d.osc2;
    s.osc2Level = d.osc2Level;
    s.osc2Semis = d.osc2Semis;
    s.osc2Cents = d.osc2Cents;
    s.sub = d.subLevel;
    s.noiseLevel = d.noiseLevel;
    s.breath = d.breath;
    s.cutoff = d.cutoff;
    s.envAmount = d.envAmount;
    s.filterDecay = d.filterDecay;
    s.sustain = d.sustain;
    s.drive = d.drive;
    s.unison = d.unison;
    s.unisonDetune = d.unisonDetune;
    for (int i = 0; i < 3; ++i)
    {
        s.fmRatio[i] = d.fm[i].modRatio;
        s.fmIndex[i] = d.fm[i].index;
        s.fmDecay[i] = d.fm[i].indexDecay;
    }
    return s;
}

namespace
{
    // the instrument as designed, reshaped by the user's settings
    InstrumentDef applySettings (const PartSettings& s)
    {
        const auto& list = instruments();
        InstrumentDef d = list[(size_t) std::clamp (s.instrument, 0, (int) list.size() - 1)];
        const float releaseMul = std::exp2 ((s.release - 0.5f) * 4.0f);

        for (int i = 0; i < d.numModes; ++i)
            if (d.modes[i].ratio > 1.01f)
                d.modes[i].amp *= s.bell;
        d.hammerBright *= s.bell;
        d.noiseAmount *= s.noise;
        d.unisonCents = s.detune;
        d.inharmonicity *= s.stretch;
        d.pickupOffset = s.pickupOffset;
        d.releaseTau *= releaseMul;
        d.release *= releaseMul;
        d.motionRate = s.rate;
        d.spread = s.width;
        d.glide = s.glide;
        d.mono = s.mono;
        d.attack = s.attack;

        d.pluckPos = s.pluckPos;
        d.bodyMix = s.body;
        d.fingerNoise = s.finger;
        d.pickupPos = s.pickupPos;
        d.pitchDrop = s.pitchDrop;
        d.mwah = s.mwah;

        std::copy (std::begin (s.drawbars), std::end (s.drawbars), d.drawbars);
        d.percussionHarmonic = s.percHarmonic;
        d.percussionDecay = s.percDecay;
        d.keyClick = s.click;
        d.chiff = s.chiff;

        d.osc1 = (Wave) std::clamp (s.osc1, 0, 3);
        d.osc2 = (Wave) std::clamp (s.osc2, 0, 3);
        d.osc2Level = s.osc2Level;
        d.osc2Semis = s.osc2Semis;
        d.osc2Cents = s.osc2Cents;
        d.subLevel = s.sub;
        d.noiseLevel = s.noiseLevel;
        d.breath = s.breath;
        d.cutoff = s.cutoff;
        d.envAmount = s.envAmount;
        d.filterDecay = s.filterDecay;
        d.sustain = s.sustain;
        d.drive = s.drive;
        d.unison = s.unison;
        d.unisonDetune = s.unisonDetune;

        for (int i = 0; i < 3; ++i)
        {
            d.fm[i].modRatio = s.fmRatio[i];
            d.fm[i].index = s.fmIndex[i];
            d.fm[i].indexDecay = s.fmDecay[i];
        }
        return d;
    }
}

void Part::prepare (float sampleRate, int maxBlock)
{
    sr = sampleRate;
    for (auto& v : voices)
        v.prepare (sampleRate);
    bufL.assign ((size_t) maxBlock, 0.0f);
    bufR.assign ((size_t) maxBlock, 0.0f);
    leslie.prepare (sampleRate);
    chorusL.prepare ((int) (0.012f * sampleRate));
    chorusR.prepare ((int) (0.012f * sampleRate));
    if (loadedInstrument < 0)
        setSettings (defaultsFor (0));
}

void Part::setSettings (const PartSettings& s)
{
    settings = s;
    if (s.instrument != loadedInstrument)
    {
        // a new instrument goes into the other slot; fading voices keep the old one
        for (auto& v : voices)
            v.quickStop();
        activeSlot = 1 - activeSlot;
        loadedInstrument = s.instrument;
        heldCount = 0;
    }
    slots[activeSlot] = applySettings (s);
}

float Part::effectiveMotion() const
{
    if (getDef().motion == Motion::none)
        return 0.0f;
    return std::clamp (settings.motion + modWheel, 0.0f, 1.0f);
}

VoiceControls Part::makeControls() const
{
    const auto& def = getDef();
    VoiceControls c;
    c.pitchBend = bend * settings.bendRange + settings.tune / 100.0f;
    c.tone = settings.tone;
    c.decayMul = std::exp2 ((settings.decay - 0.5f) * 4.0f);
    c.character = settings.character;
    c.vibrato = def.motion == Motion::vibrato ? effectiveMotion() : 0.0f;
    c.attack = settings.attack;
    return c;
}

Voice* Part::findVoice()
{
    Voice* best = nullptr;
    for (auto& v : voices)
        if (! v.active)
            return &v;

    // steal: oldest released voice first, otherwise the oldest one
    for (auto& v : voices)
        if (v.released && (best == nullptr || v.order < best->order))
            best = &v;
    if (best == nullptr)
        for (auto& v : voices)
            if (best == nullptr || v.order < best->order)
                best = &v;
    return best;
}

Voice* Part::monoVoice()
{
    Voice* current = nullptr;
    for (auto& v : voices)
        if (v.active && ! v.released && (current == nullptr || v.order > current->order))
            current = &v;
    return current;
}

void Part::startVoice (Voice& v, int key, float velocity)
{
    bool firstInPhrase = true;
    for (auto& other : voices)
        if (&other != &v && other.active && ! other.released)
            firstInPhrase = false;

    const auto& def = getDef();
    const int note = pitchFor (key);
    const float pan = def.spread * std::clamp ((float) (note - 60) / 36.0f, -1.0f, 1.0f);
    // sensitivity 0 plays every note at full strength
    const float vel = 1.0f - settings.velocity * (1.0f - velocity);
    v.order = ++counter;
    v.start (def, key, note, vel, pan, makeControls(), firstInPhrase);
}

void Part::noteOn (int key, float velocity)
{
    heldVelocity[key & 127] = velocity;

    if (getDef().mono)
    {
        // remember held keys so releasing one falls back to the previous, like a bass player
        for (int i = 0; i < heldCount; ++i)
            if (held[i] == key)
            {
                std::copy (held + i + 1, held + heldCount, held + i);
                --heldCount;
                break;
            }
        if (heldCount < 16)
            held[heldCount++] = key;

        Voice* current = monoVoice();
        if (current != nullptr && getDef().glide > 0.0f)
        {
            current->legatoTo (key, pitchFor (key));
            return;
        }
        if (current != nullptr)
            current->quickStop();
        if (auto* v = findVoice())
            startVoice (*v, key, velocity);
        return;
    }

    // re-striking a key damps the string that was still ringing
    for (auto& v : voices)
        if (v.active && v.key == key)
            v.quickStop();

    if (auto* v = findVoice())
        startVoice (*v, key, velocity);
}

void Part::noteOff (int key)
{
    if (getDef().mono)
    {
        for (int i = 0; i < heldCount; ++i)
            if (held[i] == key)
            {
                std::copy (held + i + 1, held + heldCount, held + i);
                --heldCount;
                break;
            }

        Voice* current = monoVoice();
        if (current == nullptr || current->key != key)
            return;

        if (heldCount > 0)
        {
            const int previous = held[heldCount - 1];
            if (getDef().glide > 0.0f)
                current->legatoTo (previous, pitchFor (previous));
            else
            {
                current->quickStop();
                if (auto* v = findVoice())
                    startVoice (*v, previous, heldVelocity[previous & 127]);
            }
            return;
        }
    }

    for (auto& v : voices)
        if (v.active && ! v.released && v.key == key)
        {
            if (sustainDown)
                v.sustained = true;
            else
                v.release();
        }
}

void Part::setSustain (bool down)
{
    sustainDown = down;
    if (! down)
        for (auto& v : voices)
            if (v.active && v.sustained)
                v.release();
}

void Part::allNotesOff()
{
    heldCount = 0;
    sustainDown = false;
    for (auto& v : voices)
        v.quickStop();
}

void Part::render (float* left, float* right, int n)
{
    std::fill (bufL.begin(), bufL.begin() + n, 0.0f);
    std::fill (bufR.begin(), bufR.begin() + n, 0.0f);

    const auto controls = makeControls();
    bool any = false;
    for (auto& v : voices)
        if (v.active)
        {
            v.render (bufL.data(), bufR.data(), n, controls);
            any = true;
        }

    const auto& def = getDef();
    const float motion = effectiveMotion();
    const float lfoInc = twoPi * def.motionRate / sr;

    switch (def.motion)
    {
        case Motion::stereoTremolo:
            // Suitcase amp: the signal bounces between the two speakers
            for (int i = 0; i < n; ++i)
            {
                const float s = std::sin (lfoPhase);
                lfoPhase += lfoInc;
                bufL[(size_t) i] *= 1.0f - motion * (0.5f + 0.5f * s);
                bufR[(size_t) i] *= 1.0f - motion * (0.5f - 0.5f * s);
            }
            break;
        case Motion::tremolo:
        case Motion::vibesMotor:
        {
            const float depth = def.motion == Motion::tremolo ? 0.6f : 0.5f;
            for (int i = 0; i < n; ++i)
            {
                const float g = 1.0f - motion * depth * (0.5f + 0.5f * std::sin (lfoPhase));
                lfoPhase += lfoInc;
                bufL[(size_t) i] *= g;
                bufR[(size_t) i] *= g;
            }
            break;
        }
        case Motion::leslie:
            leslie.process (bufL.data(), bufR.data(), n, motion);
            break;
        case Motion::chorus:
            // BBD chorus: ~4 ms delay swept by a slow triangle, opposite phase per side
            if (motion > 0.001f)
            {
                const float inc = def.motionRate / sr;
                for (int i = 0; i < n; ++i)
                {
                    chorusPhase += inc;
                    if (chorusPhase >= 1.0f) chorusPhase -= 1.0f;
                    const float tri = 4.0f * std::abs (chorusPhase - 0.5f) - 1.0f;
                    const float dl = (0.0045f + 0.0025f * tri) * sr;
                    const float dr = (0.0045f - 0.0025f * tri) * sr;
                    const float inL = bufL[(size_t) i], inR = bufR[(size_t) i];
                    chorusL.push (inL);
                    chorusR.push (inR);
                    bufL[(size_t) i] = inL + motion * 0.8f * chorusL.read (dl);
                    bufR[(size_t) i] = inR + motion * 0.8f * chorusR.read (dr);
                }
            }
            break;
        case Motion::vibrato:
        case Motion::none:
            break;
    }
    if (lfoPhase > twoPi)
        lfoPhase = std::fmod (lfoPhase, twoPi);

    if (! any && def.motion != Motion::leslie)
        return;

    const float gain = (settings.level / 0.8f) * (settings.level / 0.8f);
    for (int i = 0; i < n; ++i)
    {
        left[i] += bufL[(size_t) i] * gain;
        right[i] += bufR[(size_t) i] * gain;
    }
}

} // namespace bk
