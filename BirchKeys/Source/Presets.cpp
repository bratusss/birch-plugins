#include "Presets.h"

// Factory presets are *setups* of each instrument (pickup position, hammer, mallet, registration,
// strings, oscillators), never just effects. The first preset of every instrument is the
// instrument exactly as designed. Parameter ids as in PluginProcessor.cpp.

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list = {
        // ---------------- Keys
        { "Rhodes Mk I", "Rhodes Mk I", {} },
        { "Rhodes: Pickup Centred", "Rhodes Mk I", { { "pkoffset", 0.05f }, { "char", 0.45f } } },        // octave-heavy, bell-like
        { "Rhodes: Edge Growl", "Rhodes Mk I", { { "pkoffset", 0.75f }, { "char", 0.8f }, { "bell", 0.7f } } },
        { "Rhodes: Hard Tips", "Rhodes Mk I", { { "bell", 1.7f }, { "noise", 1.6f }, { "motion", 0.0f } } },
        { "Rhodes: Felt Tips", "Rhodes Mk I", { { "bell", 0.35f }, { "noise", 0.5f }, { "tone", 0.42f } } },

        { "Wurlitzer 200A", "Wurlitzer 200A", {} },
        { "Wurli: Vibrato On", "Wurlitzer 200A", { { "motion", 0.55f } } },
        { "Wurli: Hot Reeds", "Wurlitzer 200A", { { "char", 0.85f }, { "bell", 1.3f } } },
        { "Wurli: Soft Felt", "Wurlitzer 200A", { { "char", 0.25f }, { "bell", 0.5f }, { "tone", 0.42f } } },

        { "Pianet", "Pianet", {} },
        { "Pianet: Sticky Pads", "Pianet", { { "char", 0.55f }, { "decay", 0.4f } } },

        { "Grand Piano", "Grand Piano", {} },
        { "Grand: Hard Hammers", "Grand Piano", { { "char", 0.75f }, { "bell", 1.3f } } },
        { "Grand: Soft Pedal", "Grand Piano", { { "char", 0.25f }, { "bell", 0.6f }, { "velsens", 0.6f } } },
        { "Grand: Stretched Tuning", "Grand Piano", { { "stretch", 2.2f }, { "detune", 2.0f } } },

        { "Upright Piano", "Upright Piano", {} },
        { "Upright: Honky Tonk", "Upright Piano", { { "detune", 9.0f }, { "char", 0.6f } } },
        { "Upright: Felted", "Upright Piano", { { "bell", 0.3f }, { "noise", 1.6f }, { "tone", 0.4f } } },
        { "Upright: Old & Loose", "Upright Piano", { { "stretch", 1.8f }, { "detune", 5.0f }, { "decay", 0.4f } } },

        { "Clavinet D6", "Clavinet D6", {} },
        { "Clav: Bridge Pickup", "Clavinet D6", { { "pickup", 0.08f }, { "char", 0.75f } } },
        { "Clav: Neck Pickup", "Clavinet D6", { { "pickup", 0.3f }, { "tone", 0.42f } } },
        { "Clav: Muted Strings", "Clavinet D6", { { "decay", 0.22f }, { "release", 0.3f } } },

        { "DX7 E.Piano", "DX7 E.Piano", {} },
        { "DX7: Glassy", "DX7 E.Piano", { { "fm2i", 2.6f }, { "fm2d", 0.1f }, { "char", 0.65f } } },
        { "DX7: Soft Body", "DX7 E.Piano", { { "fm2i", 0.8f }, { "fm1i", 1.2f }, { "char", 0.35f } } },
        { "DX7: Bell Tines", "DX7 E.Piano", { { "fm2r", 10.0f }, { "fm2i", 2.2f }, { "fm2d", 0.25f } } },

        // ---------------- Mallets
        { "Vibraphone", "Vibraphone", {} },
        { "Vibes: Motor Off", "Vibraphone", { { "motion", 0.0f } } },
        { "Vibes: Fast Motor", "Vibraphone", { { "motion", 0.7f }, { "rate", 7.0f } } },
        { "Vibes: Soft Yarn Mallets", "Vibraphone", { { "char", 0.2f }, { "bell", 0.6f } } },
        { "Vibes: Hard Mallets", "Vibraphone", { { "char", 0.85f }, { "bell", 1.5f } } },

        { "Marimba", "Marimba", {} },
        { "Marimba: Soft Yarn", "Marimba", { { "char", 0.2f }, { "bell", 0.6f } } },
        { "Marimba: Hard Rubber", "Marimba", { { "char", 0.8f }, { "bell", 1.4f } } },

        { "Xylophone", "Xylophone", {} },
        { "Xylophone: Rubber Mallets", "Xylophone", { { "char", 0.4f }, { "bell", 0.7f } } },

        { "Glockenspiel", "Glockenspiel", {} },
        { "Glock: Brass Mallets", "Glockenspiel", { { "char", 0.85f }, { "bell", 1.4f } } },
        { "Glock: Music Box", "Glockenspiel", { { "char", 0.3f }, { "decay", 0.65f }, { "bell", 0.8f } } },

        { "Celesta", "Celesta", {} },
        { "Celesta: Bright", "Celesta", { { "char", 0.6f }, { "bell", 1.4f } } },

        { "Kalimba", "Kalimba", {} },
        { "Kalimba: Thumbnail", "Kalimba", { { "char", 0.8f }, { "bell", 1.5f } } },
        { "Kalimba: Thumb Flesh", "Kalimba", { { "char", 0.25f }, { "bell", 0.6f }, { "noise", 1.4f } } },

        // ---------------- Organ (registrations are instrument settings)
        { "Hammond B3", "Hammond B3", {} },
        { "B3: Jimmy Smith", "Hammond B3", { { "char", 0.7f }, { "motion", 0.3f } }, "888000000" },
        { "B3: Gospel Full", "Hammond B3", { { "char", 0.2f }, { "motion", 0.75f } }, "888800008" },
        { "B3: Soft Flutes", "Hammond B3", { { "char", 0.0f }, { "click", 0.3f }, { "motion", 0.3f } }, "008000000" },
        { "B3: Whistle", "Hammond B3", { { "char", 0.5f }, { "perch", 0 } }, "800008888" },
        { "B3: Full Out", "Hammond B3", { { "char", 0.3f }, { "motion", 0.85f } }, "888888888" },

        { "Vox Continental", "Vox Continental", {} },
        { "Vox: Flutes Only", "Vox Continental", { { "motion", 0.3f } }, "808000000" },
        { "Vox: All Tabs", "Vox Continental", { { "motion", 0.4f } }, "888880000" },

        { "Pipe Organ", "Pipe Organ", {} },
        { "Pipes: Soft Flute 8'", "Pipe Organ", { { "chiff", 0.3f } }, "008000000" },
        { "Pipes: Full Principal", "Pipe Organ", { { "chiff", 0.6f } }, "088868400" },

        { "Harmonium", "Harmonium", {} },
        { "Harmonium: Half Stops", "Harmonium", { { "osc2lvl", 0.0f }, { "cutoff", 1400.0f } } },

        // ---------------- Strings & winds
        { "Jazz Guitar", "Jazz Guitar", {} },
        { "Jazz Guitar: Pick", "Jazz Guitar", { { "char", 0.8f }, { "pluck", 0.12f } } },
        { "Jazz Guitar: Thumb", "Jazz Guitar", { { "char", 0.25f }, { "pluck", 0.3f }, { "tone", 0.4f } } },

        { "Nylon Guitar", "Nylon Guitar", {} },
        { "Nylon: Over the Hole", "Nylon Guitar", { { "pluck", 0.35f }, { "body", 0.6f } } },
        { "Nylon: Near the Bridge", "Nylon Guitar", { { "pluck", 0.1f }, { "char", 0.7f } } },

        { "Harp", "Harp", {} },
        { "Harp: Near the Board", "Harp", { { "pluck", 0.15f }, { "char", 0.6f } } },

        { "Soul Strings", "Soul Strings", {} },
        { "Strings: Slow Section", "Soul Strings", { { "attack", 1.1f }, { "release", 0.75f } } },
        { "Strings: Celli", "Soul Strings", { { "octave", -1.0f }, { "cutoff", 1600.0f } } },

        { "Flute", "Flute", {} },
        { "Flute: Breathy", "Flute", { { "breath", 0.7f } } },
        { "Flute: Alto", "Flute", { { "octave", -1.0f }, { "breath", 0.45f } } },

        { "Muted Trumpet", "Muted Trumpet", {} },
        { "Trumpet: Open Horn", "Muted Trumpet", { { "cutoff", 2600.0f }, { "char", 0.3f } } },

        // ---------------- Bass
        { "Upright Bass", "Upright Bass", {} },
        { "Upright: Gut Strings", "Upright Bass", { { "tone", 0.38f }, { "body", 0.6f }, { "decay", 0.4f } } },
        { "Upright: Steel, Low Action", "Upright Bass", { { "tone", 0.6f }, { "char", 0.7f }, { "finger", 0.7f } } },
        { "Upright: Slap", "Upright Bass", { { "char", 0.9f }, { "finger", 0.9f }, { "drop", 0.04f }, { "decay", 0.3f } } },

        { "P-Bass Flats", "P-Bass Flats", {} },
        { "P-Bass: Tone Open", "P-Bass Flats", { { "tone", 0.65f } } },
        { "P-Bass: Pick", "P-Bass Flats", { { "char", 0.85f }, { "pluck", 0.1f }, { "tone", 0.6f } } },
        { "P-Bass: Palm Mute", "P-Bass Flats", { { "decay", 0.25f }, { "release", 0.3f }, { "tone", 0.38f } } },

        { "Fretless Bass", "Fretless Bass", {} },
        { "Fretless: Jaco Bridge", "Fretless Bass", { { "pickup", 0.1f }, { "mwah", 0.9f }, { "char", 0.65f } } },
        { "Fretless: Neck, Smooth", "Fretless Bass", { { "pickup", 0.3f }, { "mwah", 0.4f }, { "tone", 0.4f } } },

        { "Rhodes Piano Bass", "Rhodes Piano Bass", {} },
        { "Piano Bass: Deep", "Rhodes Piano Bass", { { "pkoffset", 0.2f }, { "char", 0.35f } } },
        { "Piano Bass: Growl", "Rhodes Piano Bass", { { "pkoffset", 0.7f }, { "char", 0.85f } } },

        { "FM Bass", "FM Bass", {} },
        { "FM Bass: Lately", "FM Bass", { { "fm1i", 2.2f }, { "fm1d", 0.25f }, { "fm2i", 0.6f } } },
        { "FM Bass: Solid", "FM Bass", { { "fm1i", 4.5f }, { "fm1d", 0.08f }, { "fm2i", 2.0f } } },

        { "Moog Bass", "Moog Bass", {} },
        { "Moog: Pluck", "Moog Bass", { { "sustain", 0.2f }, { "fdecay", 0.12f }, { "envamt", 4.0f } } },
        { "Moog: Fat Square", "Moog Bass", { { "osc1", 3 }, { "osc2lvl", 0.5f }, { "cutoff", 220.0f } } },
        { "Moog: Squelch", "Moog Bass", { { "char", 0.7f }, { "envamt", 4.5f } } },

        { "808 Sub", "808 Sub", {} },
        { "808: Long Decay", "808 Sub", { { "decay", 0.8f } } },
        { "808: Short", "808 Sub", { { "decay", 0.25f } } },
        { "808: Distorted", "808 Sub", { { "drive", 3.8f } } },
        { "808: Slide", "808 Sub", { { "glide", 0.15f } } },

        { "Juno Bass", "Juno Bass", {} },
        { "Juno: Chorus Off", "Juno Bass", { { "motion", 0.0f } } },
        { "Juno: Saw Bass", "Juno Bass", { { "osc1", 2 }, { "cutoff", 600.0f } } },
    };
    return list;
}
