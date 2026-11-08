#pragma once

#include "Common.h"
#include "Instruments.h"
#include <vector>

namespace bk
{

// Values the part hands every voice once per block
struct VoiceControls
{
    float pitchBend = 0;    // semitones
    float tone = 0.5f;      // 0.5 = patch as designed
    float decayMul = 1;     // 0.25 .. 4
    float character = 0.5f; // bark / hardness / pluck / percussion / resonance / FM brightness
    float vibrato = 0;      // depth 0..1, only for Motion::vibrato instruments
    float attack = 0;       // seconds of fade-in at note start (swells), non-analog engines
};

class Voice
{
public:
    void prepare (float sampleRate);

    // key = incoming MIDI key (for note-off matching), note = pitch actually played
    void start (const InstrumentDef& def, int key, int note, float velocity, float pan,
                const VoiceControls& controls, bool firstInPhrase);
    void legatoTo (int key, int note);
    void release();
    void quickStop();
    void render (float* left, float* right, int numSamples, const VoiceControls& controls);

    bool active = false;
    bool released = false;
    bool sustained = false;  // held by the sustain pedal after note-off
    int key = -1;
    uint32_t order = 0;      // start order, for voice stealing

private:
    void startModal (const VoiceControls&);
    void startString (const VoiceControls&);
    void startOrgan (const VoiceControls&, bool firstInPhrase);
    void startAnalog (const VoiceControls&);
    void startFm (const VoiceControls&);

    void renderModal (float* left, float* right, int n, const VoiceControls&);
    void renderString (float* left, float* right, int n, const VoiceControls&);
    void renderOrgan (float* left, float* right, int n, const VoiceControls&);
    void renderAnalog (float* left, float* right, int n, const VoiceControls&);
    void renderFm (float* left, float* right, int n, const VoiceControls&);

    void updateModes (float pitchMul);
    float readLine (float delay) const;
    float vibratoSemis (const VoiceControls&, int n);
    float colour (float x);          // body formants + output low-pass, shared by modal engines
    void finishBlock (float peak, int n);
    float nextGain();

    const InstrumentDef* def = nullptr;
    float sr = 44100.0f;
    float velocity = 1, outGain = 1, panL = 0.7f, panR = 0.7f;
    float currentNote = 60, targetNote = 60;
    int ageSamples = 0, quietSamples = 0;
    float fade = 1, fadeStep = 0, swell = 1, swellStep = 0;
    Rng rng { 12345 };

    // attack noise (hammer thump, mallet click, finger)
    float noiseEnv = 0, noiseCoef = 0;
    Svf noiseFilter;

    // body colour
    Svf bodyFilter[3];
    float bodyGain[3] {};
    int numBody = 0;
    float lpState = 0, lpCoef = 0;

    // modal
    struct Mode { float ratio, tau, re, im, cr, ci; };
    static constexpr int maxModes = 56;
    Mode modes[maxModes];
    int numModes = 0;
    float modesPitch = -1;
    bool modesDamped = false;
    float deflectionScale = 0, pickupOffset = 0, pickupNorm = 1, pickupPrev = 0;
    float hpState = 0, hpPrev = 0, hpCoef = 0;

    // string
    std::vector<float> line;
    int lineMask = 0, writePos = 0;
    float loopLp = 0, loopGain = 0, damping = 0.5f, baseDelay = 100, lossDelay = 0;
    float dropEnv = 0, dropCoef = 0, t60 = 1;
    Svf pickupRes, postFilter, mwahFilter;
    float mwahAge = 0;
    DcBlocker dc;

    // organ
    float orgRe[9] {}, orgIm[9] {}, orgCr[9] {}, orgCi[9] {}, orgGain[9] {}, orgTarget[9] {};
    float orgPhase[9] {}, orgInc[9] {}, orgDrift[9] {};
    int orgContact[9] {};
    float orgRatio[9] {};
    float percRe = 0, percIm = 0, percCr = 0, percCi = 0, percEnv = 0, percCoef = 0, percRatio = 3;
    float clickEnv = 0, clickCoef = 0, chiffEnv = 0, chiffCoef = 0, orgNorm = 1, orgReleaseCoef = 0, orgAttackStep = 0;
    Svf clickFilter, chiffFilter;

    // analog
    float phase1[3] {}, phase2[3] {}, subPhase = 0;
    Adsr ampEnv;
    float filterEnv = 0, filterEnvCoef = 0, pitchEnv = 0, pitchEnvCoef = 0, breathChiff = 0, breathChiffCoef = 0;
    float glideCoef = 1;
    Ladder ladder[2];
    Svf breathFilter, formantFilter;

    // fm
    struct FmState { float cPhase, mPhase, ampEnv, indexEnv, indexFloor, indexCoef, ampCoef, level, cInc, mInc; };
    FmState fmOps[3];
    float fmNoiseEnv = 0, fmNoiseCoef = 0;

    float vibPhase = 0;
};

} // namespace bk
