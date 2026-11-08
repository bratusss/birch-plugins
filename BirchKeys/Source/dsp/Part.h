#pragma once

#include "Effects.h"
#include "Voice.h"

namespace bk
{

// Everything the user can shape about the playing instrument. defaultsFor() gives the
// values that make an instrument sound exactly as designed in Instruments.cpp.
struct PartSettings
{
    int instrument = 0;

    // common
    float level = 0.8f;
    float tone = 0.5f, decay = 0.5f, release = 0.5f; // relative, 0.5 = as designed
    float character = 0.5f, motion = 0, rate = 5;
    float velocity = 1;     // velocity sensitivity
    float width = 0.3f;     // stereo spread across the keyboard
    float attack = 0;       // seconds
    float glide = 0;        // seconds
    float tune = 0;         // cents
    int octave = 0;
    float bendRange = 2;    // semitones
    bool mono = false;

    // modal (keys, mallets)
    float bell = 1, noise = 1, detune = 0.8f, stretch = 1, pickupOffset = 0.6f;

    // string (basses, guitar, clav)
    float pluckPos = 0.2f, body = 0, finger = 0, pickupPos = 0, pitchDrop = 0, mwah = 0;

    // organ
    float drawbars[9] {};
    int percHarmonic = 3;
    float percDecay = 0.3f, click = 0.5f, chiff = 0;

    // analog
    int osc1 = 2, osc2 = 3; // Wave order: sine, triangle, saw, square
    float osc2Level = 0, osc2Semis = 0, osc2Cents = 0, sub = 0, noiseLevel = 0, breath = 0;
    float cutoff = 2000, envAmount = 0, filterDecay = 0.3f, sustain = 1, drive = 1;
    int unison = 1;
    float unisonDetune = 0;

    // fm: per pair modulator ratio, index, index decay
    float fmRatio[3] { 1, 1, 1 }, fmIndex[3] { 1, 1, 1 }, fmDecay[3] { 0.5f, 0.5f, 0.5f };
};

PartSettings defaultsFor (int instrumentIndex);

// The playable instrument: voice allocation, mono/legato, sustain pedal, motion effects
class Part
{
public:
    static constexpr int maxVoices = 16;

    void prepare (float sampleRate, int maxBlock);
    void setSettings (const PartSettings&);
    const InstrumentDef& getDef() const { return slots[activeSlot]; }

    void setPitchBend (float normalised) { bend = normalised; }  // -1 .. 1
    void setModWheel (float value) { modWheel = value; }

    void noteOn (int key, float velocity);
    void noteOff (int key);
    void setSustain (bool down);
    void allNotesOff();

    void render (float* left, float* right, int n); // adds into left/right

private:
    VoiceControls makeControls() const;
    float effectiveMotion() const;
    Voice* findVoice();
    Voice* monoVoice();
    void startVoice (Voice&, int key, float velocity);
    int pitchFor (int key) const { return std::clamp (key + 12 * settings.octave, 0, 127); }

    float sr = 44100.0f;
    // two copies, so voices still fading out keep the old instrument while the new one starts
    InstrumentDef slots[2];
    int activeSlot = 0;
    int loadedInstrument = -1;

    Voice voices[maxVoices];
    uint32_t counter = 0;
    PartSettings settings;
    float bend = 0, modWheel = 0;
    bool sustainDown = false;

    int held[16] {};
    int heldCount = 0;
    float heldVelocity[128] {};

    std::vector<float> bufL, bufR;
    Leslie leslie;
    ModDelay chorusL, chorusR;
    float lfoPhase = 0, chorusPhase = 0;
};

} // namespace bk
