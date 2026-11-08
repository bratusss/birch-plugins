#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "dsp/KeysSynth.h"

class BirchKeysProcessor : public juce::AudioProcessor
{
public:
    BirchKeysProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    // factory presets are the host's programs
    int getNumPrograms() override;
    int getCurrentProgram() override { return juce::jmax (0, currentProgram); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Picking an instrument resets its sound to the design defaults
    void selectInstrument (int index);

    // User presets: XML files in the user's app-data folder
    static juce::File userPresetFolder();
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File&);

    juce::String getPresetName() const { return presetName; }
    int getCurrentFactoryPreset() const { return currentProgram; }

    float takeOutputPeak() { return outputPeak.exchange (0.0f); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // the on-screen keyboard plays through this

    static juce::String noteName (int note); // FL Studio naming: middle C (60) = C5

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParam (const juce::String& id, float value);
    void applySettingsToParams (const bk::PartSettings&);
    bk::PartSettings readSettings() const;
    void handleMidi (const juce::MidiMessage&);

    bk::KeysSynth synth;
    std::atomic<float> outputPeak { 0.0f };
    int currentProgram = 0;   // -1 when a user preset is loaded
    juce::String presetName;
    std::vector<float> monoScratch;
    std::array<std::atomic<float>*, 96> rawParams {}; // indexed by the Id enum in the .cpp
    float value (int id) const { return rawParams[(size_t) id]->load(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchKeysProcessor)
};
