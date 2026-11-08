#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    struct Preset
    {
        const char* name;
        float grit, knock, thump, warmth, dust, wobble;
        int era;
        float pitch, lowCut, width;
        bool monoBass, sidechain;
        float mix, output;
    };

    // era: 0 = '88, 1 = '94, 2 = '98
    const Preset presets[] = {
        //  name                  grit  knock thump warm  dust  wob   era  pitch  lowcut width mono   sc     mix   out
        { "Init",                 0.35f, 0.40f, 0.30f, 0.20f, 0.20f, 0.15f, 1,  0.0f,  30.0f, 1.00f, true,  false, 1.00f,  0.0f },
        { "Golden Era",           0.45f, 0.50f, 0.35f, 0.30f, 0.25f, 0.15f, 1,  0.0f,  30.0f, 0.90f, true,  false, 1.00f,  0.0f },
        { "Dusty Basement",       0.60f, 0.45f, 0.50f, 0.40f, 0.55f, 0.25f, 0,  0.0f,  35.0f, 0.80f, true,  false, 1.00f, -1.0f },
        { "Hard Knock",           0.25f, 0.85f, 0.55f, 0.25f, 0.10f, 0.05f, 2,  0.0f,  30.0f, 1.00f, true,  false, 1.00f, -2.0f },
        { "SP Crunch",            0.85f, 0.60f, 0.40f, 0.20f, 0.15f, 0.05f, 0,  0.0f,  30.0f, 1.00f, true,  false, 1.00f, -1.0f },
        { "Crate Digger 45-33",   0.50f, 0.40f, 0.40f, 0.35f, 0.45f, 0.30f, 1, birch::rpm45to33Semitones, 30.0f, 1.00f, true, false, 1.00f, 0.0f },
        { "Vinyl Rip",            0.30f, 0.30f, 0.25f, 0.30f, 0.80f, 0.35f, 1,  0.0f,  25.0f, 1.10f, true,  false, 0.90f,  0.0f },
        { "Lo-Fi Tape",           0.40f, 0.30f, 0.30f, 0.75f, 0.30f, 0.60f, 0,  0.0f,  40.0f, 0.70f, true,  false, 1.00f,  0.0f },
        { "Clean Thump",          0.05f, 0.55f, 0.70f, 0.15f, 0.00f, 0.00f, 2,  0.0f,  25.0f, 1.00f, true,  false, 1.00f, -1.0f },
        { "Parallel Smash",       0.50f, 1.00f, 0.60f, 0.50f, 0.20f, 0.10f, 1,  0.0f,  30.0f, 1.00f, true,  false, 0.45f,  0.0f },
        { "Mono Basement",        0.55f, 0.50f, 0.45f, 0.40f, 0.40f, 0.20f, 0,  0.0f,  40.0f, 0.00f, true,  false, 1.00f,  0.0f },
        { "Kick Pump (sidechain)",0.35f, 0.80f, 0.40f, 0.30f, 0.35f, 0.15f, 1,  0.0f,  30.0f, 1.00f, true,  true,  1.00f,  0.0f },
    };

    constexpr int numPresets = (int) std::size (presets);
}

BirchBoxProcessor::BirchBoxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "BIRCHBOX", createLayout())
{
    raw.grit = apvts.getRawParameterValue ("grit");
    raw.knock = apvts.getRawParameterValue ("knock");
    raw.thump = apvts.getRawParameterValue ("thump");
    raw.warmth = apvts.getRawParameterValue ("warmth");
    raw.dust = apvts.getRawParameterValue ("dust");
    raw.wobble = apvts.getRawParameterValue ("wobble");
    raw.era = apvts.getRawParameterValue ("era");
    raw.pitch = apvts.getRawParameterValue ("pitch");
    raw.lowCut = apvts.getRawParameterValue ("lowcut");
    raw.width = apvts.getRawParameterValue ("width");
    raw.monoBass = apvts.getRawParameterValue ("monobass");
    raw.sidechain = apvts.getRawParameterValue ("sidechain");
    raw.mix = apvts.getRawParameterValue ("mix");
    raw.output = apvts.getRawParameterValue ("output");
    raw.bypass = apvts.getRawParameterValue ("bypass");

    // The standalone app has no DAW feeding it, so start with the demo beat
    if (juce::JUCEApplicationBase::isStandaloneApp())
        setSource (Source::demoBeat);
}

juce::AudioProcessorValueTreeState::ParameterLayout BirchBoxProcessor::createLayout()
{
    using Attributes = juce::AudioParameterFloatAttributes;

    auto percent = Attributes().withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; })
                               .withValueFromStringFunction ([] (const juce::String& s) { return s.getFloatValue() / 100.0f; });

    auto unit = [&] (const char* id, const char* name, float def, float max = 1.0f)
    {
        return std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
                                                            juce::NormalisableRange<float> (0.0f, max), def, percent);
    };

    juce::NormalisableRange<float> lowCutRange (20.0f, 250.0f);
    lowCutRange.setSkewForCentre (60.0f);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (unit ("grit", "Grit", 0.35f),
                unit ("knock", "Knock", 0.4f),
                unit ("thump", "Thump", 0.3f),
                unit ("warmth", "Warmth", 0.2f),
                unit ("dust", "Dust", 0.2f),
                unit ("wobble", "Wobble", 0.15f),
                std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "era", 1 }, "Era",
                                                              juce::StringArray { "'88", "'94", "'98" }, 1),
                std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "pitch", 1 }, "Pitch",
                                                             juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                             Attributes().withStringFromValueFunction ([] (float v, int)
                                                                 { return (v > 0.04f ? "+" : "") + juce::String (v, 1) + " st"; })),
                std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lowcut", 1 }, "Low Cut", lowCutRange, 30.0f,
                                                             Attributes().withStringFromValueFunction ([] (float v, int)
                                                                 { return v <= 20.5f ? juce::String ("Off") : juce::String (juce::roundToInt (v)) + " Hz"; })),
                unit ("width", "Width", 1.0f, 2.0f),
                std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "monobass", 1 }, "Mono Bass", true),
                std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "sidechain", 1 }, "Sidechain", false),
                unit ("mix", "Mix", 1.0f),
                std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "output", 1 }, "Output",
                                                             juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                                                             Attributes().withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; })),
                std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

int BirchBoxProcessor::getNumPrograms()
{
    return numPresets;
}

const juce::String BirchBoxProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, numPresets) ? presets[index].name : "";
}

void BirchBoxProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numPresets))
        return;

    currentProgram = index;
    const auto& pr = presets[index];

    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set ("grit", pr.grit);
    set ("knock", pr.knock);
    set ("thump", pr.thump);
    set ("warmth", pr.warmth);
    set ("dust", pr.dust);
    set ("wobble", pr.wobble);
    set ("era", (float) pr.era);
    set ("pitch", pr.pitch);
    set ("lowcut", pr.lowCut);
    set ("width", pr.width);
    set ("monobass", pr.monoBass ? 1.0f : 0.0f);
    set ("sidechain", pr.sidechain ? 1.0f : 0.0f);
    set ("mix", pr.mix);
    set ("output", pr.output);
}

void BirchBoxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getMainBusNumOutputChannels()));
    demo.prepare (sampleRate);
    loop.prepare (sampleRate);
    demoKick.setSize (2, samplesPerBlock);
    setLatencySamples (engine.getLatencySamples());
}

bool BirchBoxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
    }
    return true;
}

void BirchBoxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    auto main = getBusBuffer (buffer, false, 0);
    for (auto i = getMainBusNumInputChannels(); i < main.getNumChannels(); ++i)
        main.clear (i, 0, n);

    std::optional<juce::AudioBuffer<float>> hostSidechain; // non-owning view, no allocation
    const juce::AudioBuffer<float>* sidechain = nullptr;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled() && getBus (true, 1)->getNumberOfChannels() > 0)
    {
        hostSidechain.emplace (getBusBuffer (buffer, true, 1));
        sidechain = &*hostSidechain;
    }

    switch (getSource())
    {
        case Source::demoBeat:
            // With no host to route a sidechain, the demo's own kick stands in for one
            demoKick.setSize (2, n, false, false, true);
            demo.render (main, &demoKick);
            sidechain = &demoKick;
            break;
        case Source::file:
            loop.render (main);
            break;
        case Source::input:
            break;
    }

    birch::Params p;
    p.grit = raw.grit->load();
    p.knock = raw.knock->load();
    p.thump = raw.thump->load();
    p.warmth = raw.warmth->load();
    p.dust = raw.dust->load();
    p.wobble = raw.wobble->load();
    p.era = (int) raw.era->load();
    p.pitch = raw.pitch->load();
    p.lowCutHz = raw.lowCut->load();
    p.width = raw.width->load();
    p.monoBass = raw.monoBass->load() > 0.5f;
    p.sidechain = raw.sidechain->load() > 0.5f;
    p.mix = raw.mix->load();
    p.outDb = raw.output->load();
    p.bypass = raw.bypass->load() > 0.5f;

    engine.process (main, p, sidechain);

    const float peak = main.getMagnitude (0, n);
    if (peak > outputPeak.load())
        outputPeak.store (peak);
    if (engine.getGainReductionDb() > gainReduction.load())
        gainReduction.store (engine.getGainReductionDb());
}

juce::AudioProcessorEditor* BirchBoxProcessor::createEditor()
{
    return new BirchBoxEditor (*this);
}

void BirchBoxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void BirchBoxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentProgram = state.getProperty ("program", 0);
            apvts.replaceState (state);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BirchBoxProcessor();
}
