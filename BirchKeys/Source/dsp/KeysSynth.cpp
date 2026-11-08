#include "KeysSynth.h"

namespace bk
{

void KeysSynth::prepare (float sampleRate, int maxBlock)
{
    part.prepare (sampleRate, maxBlock);
    effects.prepare (sampleRate);
}

void KeysSynth::controller (int number, int value)
{
    if (number == 64)
        part.setSustain (value >= 64);
    else if (number == 1)
        part.setModWheel ((float) value / 127.0f);
    else if (number == 120 || number == 123)
        part.allNotesOff();
}

void KeysSynth::render (float* left, float* right, int n)
{
    std::fill (left, left + n, 0.0f);
    std::fill (right, right + n, 0.0f);
    part.render (left, right, n);
    effects.process (left, right, n, fx);
}

} // namespace bk
