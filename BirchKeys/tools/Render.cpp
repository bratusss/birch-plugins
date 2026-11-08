// Offline test: renders every instrument to a WAV file and prints level statistics.
// Usage: BirchKeysRender <output-dir> [instrument-index]

#include "../Source/dsp/KeysSynth.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sr)
    {
        std::ofstream f (path, std::ios::binary);
        auto u32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
        auto u16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
        const uint32_t frames = (uint32_t) l.size();
        f.write ("RIFF", 4); u32 (36 + frames * 4); f.write ("WAVE", 4);
        f.write ("fmt ", 4); u32 (16); u16 (1); u16 (2); u32 ((uint32_t) sr); u32 ((uint32_t) sr * 4); u16 (4); u16 (16);
        f.write ("data", 4); u32 (frames * 4);
        for (uint32_t i = 0; i < frames; ++i)
            for (float s : { l[i], r[i] })
            {
                const int16_t v = (int16_t) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f);
                f.write ((const char*) &v, 2);
            }
    }

    struct Event { float time; int key; float velocity; bool on; };
}

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf ("usage: %s <output-dir> [instrument-index]\n", argv[0]);
        return 1;
    }
    const std::string outDir = argv[1];
    const int only = argc > 2 ? std::atoi (argv[2]) : -1;

    constexpr int sr = 48000, block = 256;
    const auto& list = bk::instruments();

    // a Cm9 voicing with a little melody for keys, a walking line for bass (90 BPM)
    const std::vector<Event> keysEvents = {
        { 0.0f, 48, 0.75f, true }, { 0.0f, 55, 0.7f, true }, { 0.0f, 58, 0.7f, true }, { 0.0f, 62, 0.72f, true }, { 0.0f, 63, 0.65f, true },
        { 1.6f, 48, 0, false }, { 1.6f, 55, 0, false }, { 1.6f, 58, 0, false }, { 1.6f, 62, 0, false }, { 1.6f, 63, 0, false },
        { 2.0f, 67, 0.95f, true }, { 2.3f, 67, 0, false }, { 2.33f, 70, 0.6f, true }, { 2.66f, 70, 0, false },
        { 2.66f, 72, 1.0f, true }, { 3.6f, 72, 0, false },
    };
    const std::vector<Event> bassEvents = {
        { 0.0f, 36, 0.9f, true }, { 0.6f, 36, 0, false }, { 0.667f, 36, 0.6f, true }, { 0.95f, 36, 0, false },
        { 1.333f, 43, 0.85f, true }, { 1.9f, 43, 0, false }, { 2.0f, 41, 0.8f, true }, { 2.6f, 41, 0, false },
        { 2.667f, 39, 0.9f, true }, { 3.6f, 39, 0, false },
    };

    for (int index = 0; index < (int) list.size(); ++index)
    {
        if (only >= 0 && index != only)
            continue;
        const auto& def = list[(size_t) index];

        bk::KeysSynth synth;
        synth.prepare ((float) sr, block);
        synth.part.setSettings (bk::defaultsFor (index));
        synth.fx = {};

        const auto& events = def.isBass ? bassEvents : keysEvents;
        const int total = (int) (4.5f * sr);
        std::vector<float> l ((size_t) total), r ((size_t) total);
        size_t next = 0;
        bool bad = false;

        for (int pos = 0; pos < total; pos += block)
        {
            const int n = std::min (block, total - pos);
            while (next < events.size() && events[next].time * sr <= (float) pos)
            {
                const auto& e = events[next++];
                if (e.on) synth.noteOn (e.key, e.velocity);
                else synth.noteOff (e.key);
            }
            synth.render (l.data() + pos, r.data() + pos, n);
        }

        float peak = 0.0f;
        double sum = 0.0;
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (l[(size_t) i]) || ! std::isfinite (r[(size_t) i]))
                bad = true;
            peak = std::max (peak, std::max (std::abs (l[(size_t) i]), std::abs (r[(size_t) i])));
            sum += 0.5 * (l[(size_t) i] * l[(size_t) i] + r[(size_t) i] * r[(size_t) i]);
        }
        const double rmsDb = 10.0 * std::log10 (sum / total + 1e-12);

        std::string file = def.name;
        for (auto& ch : file)
            if (ch == ' ') ch = '_';
        char prefix[8];
        std::snprintf (prefix, sizeof prefix, "%02d_", index);
        writeWav (outDir + "/" + prefix + file + ".wav", l, r, sr);
        std::printf ("%2d %-20s peak %6.3f  rms %6.1f dB %s\n", index, def.name, peak, rmsDb, bad ? "  NaN/Inf!" : "");
    }
    return 0;
}
