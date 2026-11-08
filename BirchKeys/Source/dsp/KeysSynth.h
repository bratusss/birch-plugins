#pragma once

#include "Part.h"

namespace bk
{

// One instrument at a time, then the sampler FX chain
class KeysSynth
{
public:
    void prepare (float sampleRate, int maxBlock);

    void noteOn (int key, float velocity) { part.noteOn (key & 127, velocity); }
    void noteOff (int key) { part.noteOff (key & 127); }
    void controller (int number, int value);
    void pitchBend (int value14bit) { part.setPitchBend ((float) (value14bit - 8192) / 8192.0f); }
    void allNotesOff() { part.allNotesOff(); }

    // renders (overwrites) n samples
    void render (float* left, float* right, int n);

    Part part;
    FxParams fx;

private:
    KeysFx effects;
};

} // namespace bk
