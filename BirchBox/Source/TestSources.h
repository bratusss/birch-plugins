#pragma once

#include <juce_audio_formats/juce_audio_formats.h>

// Signal sources for trying Birch Box without a DAW: a built-in drum loop and a looped audio file.

class DemoBeat
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        position = 0;
        nextStep = 0;
        kick.t = snare.t = hat.t = 1.0e9;
    }

    // kickOnly (may be null) receives just the kick, to stand in for a sidechain feed
    void render (juce::AudioBuffer<float>& buffer, juce::AudioBuffer<float>* kickOnly)
    {
        const double stepLen = sr * 60.0 / bpm / 4.0;
        const double loopLen = stepLen * numSteps;
        const double dt = 1.0 / sr;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            while (nextStep < numSteps && position >= stepStart (nextStep, stepLen))
                trigger (nextStep++);

            float s = 0.0f;

            if (kick.t < 1.2)
            {
                const double f = 48.0 + 120.0 * std::exp (-kick.t / 0.03);
                kick.phase += juce::MathConstants<double>::twoPi * f * dt;
                s = std::tanh (1.8f * (float) std::sin (kick.phase)) * (float) std::exp (-kick.t / 0.32) * kick.vel * 0.8f;
                kick.t += dt;
            }

            if (kickOnly != nullptr)
                for (int ch = 0; ch < kickOnly->getNumChannels(); ++ch)
                    kickOnly->setSample (ch, i, s * 0.7f);

            if (snare.t < 0.6)
            {
                const float tone = (float) (std::sin (juce::MathConstants<double>::twoPi * 190.0 * snare.t) * std::exp (-snare.t / 0.05));
                const float noise = (rng.nextFloat() * 2.0f - 1.0f) * (float) std::exp (-snare.t / 0.15);
                s += (0.45f * tone + 0.5f * noise) * snare.vel;
                snare.t += dt;
            }

            if (hat.t < 0.3)
            {
                const float white = rng.nextFloat() * 2.0f - 1.0f;
                s += (white - hatPrev) * 0.5f * (float) std::exp (-hat.t / 0.03) * 0.22f * hat.vel;
                hatPrev = white;
                hat.t += dt;
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample (ch, i, s * 0.7f);

            position += 1.0;
            if (position >= loopLen)
            {
                position -= loopLen;
                nextStep = 0;
            }
        }
    }

private:
    struct Voice { double t = 1.0e9, phase = 0; float vel = 1; };

    static constexpr int numSteps = 32; // two bars of 16ths
    static constexpr double bpm = 90.0;
    static constexpr double swing = 0.18; // fraction of a 16th that off-beats are pushed late

    // x = full hit, o = medium, g = ghost note
    static constexpr const char* kickPattern  = "x......o..x....." "x........xx.....";
    static constexpr const char* snarePattern = "....x.......x..g" "....x.......x...";
    static constexpr const char* hatPattern   = "x.o.x.o.x.o.x.o." "x.o.x.o.x.o.x.oo";

    static double stepStart (int step, double stepLen) { return (step + ((step & 1) ? swing : 0.0)) * stepLen; }

    static float velocity (char c) { return c == 'x' ? 1.0f : c == 'o' ? 0.6f : c == 'g' ? 0.3f : 0.0f; }

    void trigger (int step)
    {
        auto start = [this] (Voice& v, float vel)
        {
            if (vel <= 0.0f)
                return;
            v.t = 0;
            v.phase = 0;
            v.vel = vel * (0.92f + 0.08f * rng.nextFloat());
        };
        start (kick, velocity (kickPattern[step]));
        start (snare, velocity (snarePattern[step]));
        start (hat, velocity (hatPattern[step]));
    }

    double sr = 44100.0, position = 0;
    int nextStep = 0;
    Voice kick, snare, hat;
    float hatPrev = 0;
    juce::Random rng;
};

class LoopPlayer
{
public:
    void prepare (double sampleRate) { hostRate = sampleRate; }

    // Called on the message thread
    bool load (const juce::File& file)
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples <= 0)
            return false;

        const int length = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 120.0));
        juce::AudioBuffer<float> audio (juce::jlimit (1, 2, (int) reader->numChannels), length);
        reader->read (&audio, 0, length, 0, true, true);

        {
            const juce::SpinLock::ScopedLockType lock (mutex);
            data = std::move (audio);
            fileRate = reader->sampleRate;
            position = 0;
        }
        name = file.getFileName();
        return true;
    }

    bool isLoaded() const { return name.isNotEmpty(); }
    juce::String getName() const { return name; }

    void render (juce::AudioBuffer<float>& buffer)
    {
        const juce::SpinLock::ScopedTryLockType lock (mutex);
        const int length = data.getNumSamples();
        if (! lock.isLocked() || length < 2)
        {
            buffer.clear();
            return;
        }

        const double inc = fileRate / hostRate;
        const int fileChannels = data.getNumChannels();

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const int i0 = (int) position;
            const int i1 = (i0 + 1) % length;
            const float frac = (float) (position - i0);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const float* d = data.getReadPointer (juce::jmin (ch, fileChannels - 1));
                buffer.setSample (ch, i, d[i0] + frac * (d[i1] - d[i0]));
            }

            position += inc;
            if (position >= length)
                position -= length;
        }
    }

private:
    juce::SpinLock mutex;
    juce::AudioBuffer<float> data;
    double fileRate = 44100.0, hostRate = 44100.0, position = 0;
    juce::String name;
};
