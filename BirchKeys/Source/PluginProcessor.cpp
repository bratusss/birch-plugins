#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    // every parameter, in one place; the order matches the Id enum
    enum Id
    {
        inst, level, tone, decay, release, character, motion, rate, velsens, width, attack, glide, tune, octave, bend, mono,
        bell, noise, detune, stretch, pkoffset,
        pluck, body, finger, pickup, drop, mwah,
        db1, db2, db3, db4, db5, db6, db7, db8, db9, perch, percdecay, click, chiff,
        osc1, osc2, osc2lvl, osc2semi, osc2cents, sub, noiselvl, breath, cutoff, envamt, fdecay, sustain, drive, unison, unidetune,
        fm1r, fm1i, fm1d, fm2r, fm2i, fm2d, fm3r, fm3i, fm3d,
        grit, era, filter, reso, warmth, wobble, dust, room, volume,
        numIds
    };

    const char* const ids[numIds] = {
        "inst", "level", "tone", "decay", "release", "char", "motion", "rate", "velsens", "width", "attack", "glide", "tune", "octave", "bend", "mono",
        "bell", "noise", "detune", "stretch", "pkoffset",
        "pluck", "body", "finger", "pickup", "drop", "mwah",
        "db1", "db2", "db3", "db4", "db5", "db6", "db7", "db8", "db9", "perch", "percdecay", "click", "chiff",
        "osc1", "osc2", "osc2lvl", "osc2semi", "osc2cents", "sub", "noiselvl", "breath", "cutoff", "envamt", "fdecay", "sustain", "drive", "unison", "unidetune",
        "fm1r", "fm1i", "fm1d", "fm2r", "fm2i", "fm2d", "fm3r", "fm3i", "fm3d",
        "grit", "era", "filter", "reso", "warmth", "wobble", "dust", "room", "volume",
    };

    int instrumentIndex (const char* name)
    {
        const auto& list = bk::instruments();
        for (size_t i = 0; i < list.size(); ++i)
            if (juce::String (list[i].name) == name)
                return (int) i;
        jassertfalse;
        return 0;
    }

    // sampler chain every preset starts from
    const std::pair<Id, float> fxDefaults[] = {
        { grit, 0.2f }, { era, 1.0f }, { filter, 1.0f }, { reso, 0.15f }, { warmth, 0.2f },
        { wobble, 0.08f }, { dust, 0.12f }, { room, 0.2f }, { volume, 0.0f },
    };
}

juce::String BirchKeysProcessor::noteName (int note)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (names[note % 12]) + juce::String (note / 12);
}

BirchKeysProcessor::BirchKeysProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "BIRCHKEYS", createLayout())
{
    static_assert (numIds <= 96, "rawParams is too small");
    for (int i = 0; i < numIds; ++i)
    {
        rawParams[(size_t) i] = apvts.getRawParameterValue (ids[i]);
        jassert (rawParams[(size_t) i] != nullptr);
    }
    presetName = factoryPresets().front().name;
}

juce::AudioProcessorValueTreeState::ParameterLayout BirchKeysProcessor::createLayout()
{
    using Float = juce::AudioParameterFloat;
    using Attributes = juce::AudioParameterFloatAttributes;
    const auto d = bk::defaultsFor (0);

    auto text = [] (std::function<juce::String (float)> f) { return Attributes().withStringFromValueFunction ([f] (float v, int) { return f (v); }); };
    auto percent = text ([] (float v) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    auto seconds = text ([] (float v) { return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms" : juce::String (v, 2) + " s"; });
    auto hertz = text ([] (float v) { return v < 1000.0f ? juce::String (v, v < 10.0f ? 2 : 0) + " Hz" : juce::String (v / 1000.0f, 2) + " kHz"; });
    auto times = text ([] (float v) { return juce::String (v, 2) + "x"; });
    auto cents = text ([] (float v) { return juce::String (v, 1) + " ct"; });

    auto range = [] (float lo, float hi, float centre = -1.0f)
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (centre > lo)
            r.setSkewForCentre (centre);
        return r;
    };
    auto add = [] (juce::AudioProcessorValueTreeState::ParameterLayout& l, Id id, const juce::String& name,
                   juce::NormalisableRange<float> r, float def, Attributes attrs)
    {
        l.add (std::make_unique<Float> (juce::ParameterID { ids[id], 1 }, name, r, def, attrs));
    };
    auto addInt = [] (juce::AudioProcessorValueTreeState::ParameterLayout& l, Id id, const juce::String& name, int lo, int hi, int def)
    {
        l.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ids[id], 1 }, name, lo, hi, def));
    };
    auto addChoice = [] (juce::AudioProcessorValueTreeState::ParameterLayout& l, Id id, const juce::String& name, const juce::StringArray& items, int def)
    {
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ids[id], 1 }, name, items, def));
    };

    juce::StringArray names;
    for (const auto& def : bk::instruments())
        names.add (def.name);
    const juce::StringArray waves { "Sine", "Triangle", "Saw", "Square" };

    juce::AudioProcessorValueTreeState::ParameterLayout l;
    addChoice (l, inst, "Instrument", names, 0);
    add (l, level, "Level", range (0, 1), 0.8f, percent);
    add (l, tone, "Tone", range (0, 1), 0.5f, percent);
    add (l, decay, "Decay", range (0, 1), 0.5f, percent);
    add (l, release, "Release", range (0, 1), 0.5f, percent);
    add (l, character, "Character", range (0, 1), d.character, percent);
    add (l, motion, "Motion", range (0, 1), d.motion, percent);
    add (l, rate, "Motion Rate", range (0.2f, 12.0f, 4.0f), d.rate, hertz);
    add (l, velsens, "Velocity", range (0, 1), d.velocity, percent);
    add (l, width, "Width", range (0, 1), d.width, percent);
    add (l, attack, "Attack", range (0.0f, 3.0f, 0.3f), d.attack, seconds);
    add (l, glide, "Glide", range (0.0f, 0.5f, 0.08f), d.glide, seconds);
    add (l, tune, "Fine Tune", range (-100.0f, 100.0f), 0.0f, cents);
    addInt (l, octave, "Octave", -2, 2, 0);
    addInt (l, bend, "Bend Range", 0, 12, 2);
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ids[mono], 1 }, "Mono", d.mono));

    add (l, bell, "Bell", range (0.0f, 2.0f), d.bell, times);
    add (l, noise, "Attack Noise", range (0.0f, 2.0f), d.noise, times);
    add (l, detune, "Unison Detune", range (0.0f, 12.0f), d.detune, cents);
    add (l, stretch, "Stretch", range (0.0f, 3.0f), d.stretch, times);
    add (l, pkoffset, "Pickup Offset", range (0, 1), d.pickupOffset, percent);

    add (l, pluck, "Pluck Position", range (0.02f, 0.5f), d.pluckPos, percent);
    add (l, body, "Body", range (0, 1), d.body, percent);
    add (l, finger, "Finger Noise", range (0, 1), d.finger, percent);
    add (l, pickup, "Pickup Position", range (0.0f, 0.5f), d.pickupPos, percent);
    add (l, drop, "Pitch Drop", range (0.0f, 0.05f), d.pitchDrop, percent);
    add (l, mwah, "Mwah", range (0, 1), d.mwah, percent);

    const char* footages[] = { "16'", "5 1/3'", "8'", "4'", "2 2/3'", "2'", "1 3/5'", "1 1/3'", "1'" };
    for (int i = 0; i < 9; ++i)
        addInt (l, (Id) (db1 + i), juce::String ("Drawbar ") + footages[i], 0, 8, (int) d.drawbars[i]);
    addChoice (l, perch, "Percussion Harmonic", { "2nd", "3rd" }, d.percHarmonic == 2 ? 0 : 1);
    add (l, percdecay, "Percussion Decay", range (0.05f, 1.5f, 0.3f), d.percDecay, seconds);
    add (l, click, "Key Click", range (0, 1), d.click, percent);
    add (l, chiff, "Chiff", range (0, 1), d.chiff, percent);

    addChoice (l, osc1, "Osc 1 Wave", waves, d.osc1);
    addChoice (l, osc2, "Osc 2 Wave", waves, d.osc2);
    add (l, osc2lvl, "Osc 2 Level", range (0, 1), d.osc2Level, percent);
    addInt (l, osc2semi, "Osc 2 Semitones", -24, 24, (int) d.osc2Semis);
    add (l, osc2cents, "Osc 2 Detune", range (-50.0f, 50.0f), d.osc2Cents, cents);
    add (l, sub, "Sub Osc", range (0, 1), d.sub, percent);
    add (l, noiselvl, "Noise", range (0, 1), d.noiseLevel, percent);
    add (l, breath, "Breath", range (0, 1), d.breath, percent);
    add (l, cutoff, "Cutoff", range (30.0f, 12000.0f, 800.0f), d.cutoff, hertz);
    add (l, envamt, "Filter Env", range (0.0f, 6.0f), d.envAmount, text ([] (float v) { return juce::String (v, 1) + " oct"; }));
    add (l, fdecay, "Filter Decay", range (0.02f, 3.0f, 0.4f), d.filterDecay, seconds);
    add (l, sustain, "Sustain", range (0, 1), d.sustain, percent);
    add (l, drive, "Drive", range (0.5f, 4.0f), d.drive, times);
    addInt (l, unison, "Unison", 1, 3, d.unison);
    add (l, unidetune, "Unison Spread", range (0.0f, 40.0f), d.unisonDetune, cents);

    for (int i = 0; i < 3; ++i)
    {
        const juce::String n = juce::String (i + 1);
        add (l, (Id) (fm1r + 3 * i), "FM " + n + " Ratio", range (0.25f, 16.0f, 2.0f), d.fmRatio[i], times);
        add (l, (Id) (fm1i + 3 * i), "FM " + n + " Index", range (0.0f, 6.0f), d.fmIndex[i], times);
        add (l, (Id) (fm1d + 3 * i), "FM " + n + " Index Decay", range (0.01f, 3.0f, 0.3f), d.fmDecay[i], seconds);
    }

    add (l, grit, "Grit", range (0, 1), 0.2f, percent);
    addChoice (l, era, "Era", { "'88", "'94", "'98" }, 1);
    add (l, filter, "SP Filter", range (0, 1), 1.0f, percent);
    add (l, reso, "Resonance", range (0, 1), 0.15f, percent);
    add (l, warmth, "Warmth", range (0, 1), 0.2f, percent);
    add (l, wobble, "Wobble", range (0, 1), 0.08f, percent);
    add (l, dust, "Dust", range (0, 1), 0.12f, percent);
    add (l, room, "Room", range (0, 1), 0.2f, percent);
    add (l, volume, "Volume", range (-24.0f, 6.0f), 0.0f, text ([] (float v) { return juce::String (v, 1) + " dB"; }));
    return l;
}

//==============================================================================
void BirchKeysProcessor::setParam (const juce::String& id, float v)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (v));
}

void BirchKeysProcessor::applySettingsToParams (const bk::PartSettings& s)
{
    auto set = [this] (Id id, float v) { setParam (ids[id], v); };
    set (inst, (float) s.instrument);
    set (level, s.level);
    set (tone, s.tone);
    set (decay, s.decay);
    set (release, s.release);
    set (character, s.character);
    set (motion, s.motion);
    set (rate, s.rate);
    set (velsens, s.velocity);
    set (width, s.width);
    set (attack, s.attack);
    set (glide, s.glide);
    set (tune, s.tune);
    set (octave, (float) s.octave);
    set (bend, s.bendRange);
    set (mono, s.mono ? 1.0f : 0.0f);
    set (bell, s.bell);
    set (noise, s.noise);
    set (detune, s.detune);
    set (stretch, s.stretch);
    set (pkoffset, s.pickupOffset);
    set (pluck, s.pluckPos);
    set (body, s.body);
    set (finger, s.finger);
    set (pickup, s.pickupPos);
    set (drop, s.pitchDrop);
    set (mwah, s.mwah);
    for (int i = 0; i < 9; ++i)
        set ((Id) (db1 + i), s.drawbars[i]);
    set (perch, s.percHarmonic == 2 ? 0.0f : 1.0f);
    set (percdecay, s.percDecay);
    set (click, s.click);
    set (chiff, s.chiff);
    set (osc1, (float) s.osc1);
    set (osc2, (float) s.osc2);
    set (osc2lvl, s.osc2Level);
    set (osc2semi, s.osc2Semis);
    set (osc2cents, s.osc2Cents);
    set (sub, s.sub);
    set (noiselvl, s.noiseLevel);
    set (breath, s.breath);
    set (cutoff, s.cutoff);
    set (envamt, s.envAmount);
    set (fdecay, s.filterDecay);
    set (sustain, s.sustain);
    set (drive, s.drive);
    set (unison, (float) s.unison);
    set (unidetune, s.unisonDetune);
    for (int i = 0; i < 3; ++i)
    {
        set ((Id) (fm1r + 3 * i), s.fmRatio[i]);
        set ((Id) (fm1i + 3 * i), s.fmIndex[i]);
        set ((Id) (fm1d + 3 * i), s.fmDecay[i]);
    }
}

bk::PartSettings BirchKeysProcessor::readSettings() const
{
    bk::PartSettings s;
    s.instrument = (int) value (inst);
    s.level = value (level);
    s.tone = value (tone);
    s.decay = value (decay);
    s.release = value (release);
    s.character = value (character);
    s.motion = value (motion);
    s.rate = value (rate);
    s.velocity = value (velsens);
    s.width = value (width);
    s.attack = value (attack);
    s.glide = value (glide);
    s.tune = value (tune);
    s.octave = (int) value (octave);
    s.bendRange = value (bend);
    s.mono = value (mono) > 0.5f;
    s.bell = value (bell);
    s.noise = value (noise);
    s.detune = value (detune);
    s.stretch = value (stretch);
    s.pickupOffset = value (pkoffset);
    s.pluckPos = value (pluck);
    s.body = value (body);
    s.finger = value (finger);
    s.pickupPos = value (pickup);
    s.pitchDrop = value (drop);
    s.mwah = value (mwah);
    for (int i = 0; i < 9; ++i)
        s.drawbars[i] = value ((Id) (db1 + i));
    s.percHarmonic = value (perch) < 0.5f ? 2 : 3;
    s.percDecay = value (percdecay);
    s.click = value (click);
    s.chiff = value (chiff);
    s.osc1 = (int) value (osc1);
    s.osc2 = (int) value (osc2);
    s.osc2Level = value (osc2lvl);
    s.osc2Semis = value (osc2semi);
    s.osc2Cents = value (osc2cents);
    s.sub = value (sub);
    s.noiseLevel = value (noiselvl);
    s.breath = value (breath);
    s.cutoff = value (cutoff);
    s.envAmount = value (envamt);
    s.filterDecay = value (fdecay);
    s.sustain = value (sustain);
    s.drive = value (drive);
    s.unison = (int) value (unison);
    s.unisonDetune = value (unidetune);
    for (int i = 0; i < 3; ++i)
    {
        s.fmRatio[i] = value ((Id) (fm1r + 3 * i));
        s.fmIndex[i] = value ((Id) (fm1i + 3 * i));
        s.fmDecay[i] = value ((Id) (fm1d + 3 * i));
    }
    return s;
}

void BirchKeysProcessor::selectInstrument (int index)
{
    applySettingsToParams (bk::defaultsFor (index));
    // the instrument's own first preset is its design default
    const auto& presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        if (instrumentIndex (presets[i].instrument) == index)
        {
            currentProgram = (int) i;
            presetName = presets[i].name;
            break;
        }
}

//==============================================================================
int BirchKeysProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String BirchKeysProcessor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? presets[(size_t) index].name : "";
}

void BirchKeysProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size()))
        return;
    const auto& p = presets[(size_t) index];

    applySettingsToParams (bk::defaultsFor (instrumentIndex (p.instrument)));
    for (auto [id, v] : fxDefaults)
        setParam (ids[id], v);
    if (p.drawbars != nullptr)
        for (int i = 0; i < 9; ++i)
            setParam (ids[db1 + i], (float) (p.drawbars[i] - '0'));
    for (const auto& [id, v] : p.values)
        setParam (id, v);

    currentProgram = index;
    presetName = p.name;
}

juce::File BirchKeysProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Birch Keys").getChildFile ("Presets");
}

juce::Array<juce::File> BirchKeysProcessor::getUserPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.bkpreset");
    files.sort();
    return files;
}

bool BirchKeysProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty())
        return false;
    auto folder = userPresetFolder();
    folder.createDirectory();
    auto state = apvts.copyState();
    state.setProperty ("presetName", name.trim(), nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (folder.getChildFile (clean + ".bkpreset")))
        {
            currentProgram = -1;
            presetName = name.trim();
            return true;
        }
    return false;
}

bool BirchKeysProcessor::loadUserPreset (const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse (file))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            presetName = state.getProperty ("presetName", file.getFileNameWithoutExtension()).toString();
            apvts.replaceState (state);
            currentProgram = -1;
            return true;
        }
    return false;
}

//==============================================================================
void BirchKeysProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.prepare ((float) sampleRate, juce::jmax (16, samplesPerBlock));
    monoScratch.assign ((size_t) juce::jmax (16, samplesPerBlock), 0.0f);
    keyboardState.reset();
}

bool BirchKeysProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void BirchKeysProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        synth.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        synth.noteOff (m.getNoteNumber());
    else if (m.isController())
        synth.controller (m.getControllerNumber(), m.getControllerValue());
    else if (m.isPitchWheel())
        synth.pitchBend (m.getPitchWheelValue());
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        synth.allNotesOff();
}

void BirchKeysProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    synth.part.setSettings (readSettings());
    auto& fx = synth.fx;
    fx.grit = value (grit);
    fx.era = (int) value (era);
    fx.filter = value (filter);
    fx.resonance = value (reso);
    fx.warmth = value (warmth);
    fx.wobble = value (wobble);
    fx.dust = value (dust);
    fx.room = value (room);
    fx.volumeDb = value (volume);

    float* left = buffer.getWritePointer (0);
    const bool isMono = buffer.getNumChannels() < 2;
    const int maxChunk = (int) monoScratch.size();

    // render between MIDI events, in pieces no longer than the prepared block size
    int pos = 0;
    auto renderTo = [&] (int end)
    {
        while (pos < end)
        {
            const int len = juce::jmin (end - pos, maxChunk);
            if (isMono)
            {
                synth.render (left + pos, monoScratch.data(), len);
                for (int i = 0; i < len; ++i)
                    left[pos + i] = 0.5f * (left[pos + i] + monoScratch[(size_t) i]);
            }
            else
            {
                synth.render (left + pos, buffer.getWritePointer (1) + pos, len);
            }
            pos += len;
        }
    };

    for (const auto metadata : midi)
    {
        renderTo (juce::jlimit (0, n, metadata.samplePosition));
        handleMidi (metadata.getMessage());
    }
    renderTo (n);

    const float peak = buffer.getMagnitude (0, n);
    if (peak > outputPeak.load())
        outputPeak.store (peak);
}

juce::AudioProcessorEditor* BirchKeysProcessor::createEditor()
{
    return new BirchKeysEditor (*this);
}

void BirchKeysProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BirchKeysProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = state.getProperty ("program", 0);
            presetName = state.getProperty ("presetName", presetName).toString();
            apvts.replaceState (state);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BirchKeysProcessor();
}
