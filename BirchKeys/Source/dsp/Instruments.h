#pragma once

#include <vector>

namespace bk
{

enum class Engine { modal, string, organ, analog, fm };

// What the per-part MOTION control does for this instrument
enum class Motion { none, stereoTremolo, tremolo, vibesMotor, leslie, vibrato, chorus };

// Transducer after the vibrating part (see RESEARCH.md)
enum class Pickup { none, magnetic, electrostatic };

enum class Wave { sine, triangle, saw, square };

// A decaying partial of a struck bar / tine / reed
struct ModeDef
{
    float ratio = 1;      // frequency relative to the fundamental
    float amp = 1;
    float decay = 1;      // time constant in seconds at middle C
    float velBright = 0;  // 0 = same at every velocity, 1 = only hard hits excite it
};

struct Formant
{
    float hz = 0, q = 1, gain = 0; // gain 0 = unused
};

// One FM operator pair (carrier + modulator), DX7 style
struct FmPair
{
    float carrierRatio = 1, modRatio = 1;
    float level = 1;            // carrier output level
    float index = 1;            // modulation index at the start
    float indexFloor = 0;       // index it decays to while the key is held
    float indexDecay = 0.5f;    // seconds
    float ampDecay = 2;         // seconds (time constant)
    float velIndex = 0.5f;      // how much velocity raises the index
    float detuneCents = 0;
};

// One instrument. Defaults are neutral; each instrument sets what it needs.
struct InstrumentDef
{
    const char* name = "";
    const char* category = "";
    const char* physics = "";     // one-line description shown in the UI
    Engine engine = Engine::modal;
    bool isBass = false;
    float gain = 1;
    float spread = 0.3f;          // stereo spread across the keyboard
    float decayKeytrack = 0.75f;  // decay multiplier per octave up
    float releaseTau = 0.1f;      // damper time constant
    bool mono = false;
    float glide = 0;
    float characterDefault = 0.5f;
    Motion motion = Motion::none;
    float motionRate = 5;
    float motionDefault = 0;
    float attack = 0;             // swell-in for non-analog engines; analog uses its own ADSR

    // ---- modal: tine / reed / bar / string
    ModeDef modes[8];
    int numModes = 0;
    float transientMs = 0;        // bar/tine partials above 1.0 only live this long (0 = own decay)
    float tonebarRatio = 0;       // Rhodes: tonebar eigenfrequency relative to f0 (only in the transient)
    bool pianoPartials = false;
    float inharmonicity = 0.0004f;
    float hammerPos = 0.12f;
    float hammerBright = 1;       // hammer felt hardness
    float unisonCents = 0.8f;
    float promptShare = 0.7f;
    float promptRatio = 0.25f;
    float baseDecay = 3;
    int maxPartials = 20;
    Pickup pickup = Pickup::none;
    float pickupOffset = 0.6f;    // 0 = tine centred on the magnet (octave), 1 = at the edge
    float deflection = 1;         // how far the tine swings relative to the pickup field width
    float noiseAmount = 0, noiseDecay = 0.01f, noiseFreq = 1500;
    Formant body[3];              // resonator / soundboard / cabinet colour
    float lowpass = 0;            // output low-pass (0 = none)

    // ---- string (waveguide)
    float t60 = 2;
    float stringDamping = 0.45f;
    float pluckPos = 0.2f;
    float pluckWidth = 0.08f;
    float pluckNoise = 0.2f;
    float pickupPos = 0;
    float pickupResHz = 0, pickupResQ = 1.5f;   // magnetic pickup LC resonance (0 = none)
    float pitchDrop = 0;
    float bodyMix = 0;
    float fingerNoise = 0;
    float postLowpass = 12000;
    float mwah = 0;               // fretless: sweeping formant that follows the note's envelope

    // ---- organ
    float drawbars[9] {};
    Wave organWave = Wave::sine;  // tonewheel = sine, transistor combo = saw/square mix
    float organWaveMix = 0;       // 0 = pure sine, 1 = full organWave
    int percussionHarmonic = 3;
    float percussionDecay = 0.3f;
    float keyClick = 0.5f;
    float chiff = 0;              // pipe speech noise
    float pipeDrift = 0;          // per-pipe random detune in cents
    float organAttack = 0.0005f;

    // ---- analog
    Wave osc1 = Wave::saw, osc2 = Wave::square;
    float osc2Level = 0, osc2Semis = 0, osc2Cents = 0;
    int unison = 1;
    float unisonDetune = 0;
    float pulseWidth = 0.5f;
    float subLevel = 0, noiseLevel = 0, breath = 0;
    float cutoff = 2000, keytrack = 0.5f, envAmount = 0, filterDecay = 0.3f, resonance = 0.2f;
    float decay = 0.5f, sustain = 1, release = 0.1f;
    float pitchEnvSemis = 0, pitchEnvDecay = 0.05f;
    float drive = 1;
    float vibratoDelay = 0.3f;
    Formant formant;              // mute / horn colour
    float resonantPitchHz = 0;    // 808: the body is a ringing filter at this frequency, keytracked

    // ---- fm
    FmPair fm[3];
    int numFm = 0;
    float fmNoise = 0;            // key-off / hammer noise burst
};

const std::vector<InstrumentDef>& instruments();

} // namespace bk
