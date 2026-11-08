#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "BirchEngine.h"
#include "TestSources.h"

class BirchBoxProcessor : public juce::AudioProcessor
{
public:
    enum class Source { input = 0, demoBeat, file };

    BirchBoxProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.25; }

    // Factory presets are exposed as programs, so FL Studio's preset menu lists them too
    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Test sources (not saved with the project)
    void setSource (Source s) { source.store ((int) s); }
    Source getSource() const { return (Source) source.load(); }
    bool loadLoopFile (const juce::File& file) { return loop.load (file); }
    bool hasLoopFile() const { return loop.isLoaded(); }
    juce::String getLoopFileName() const { return loop.getName(); }

    float takeOutputPeak() { return outputPeak.exchange (0.0f); }
    float takeGainReduction() { return gainReduction.exchange (0.0f); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    birch::Engine engine;
    DemoBeat demo;
    LoopPlayer loop;
    juce::AudioBuffer<float> demoKick;
    std::atomic<int> source { (int) Source::input };
    std::atomic<float> outputPeak { 0.0f }, gainReduction { 0.0f };
    int currentProgram = 0;

    struct
    {
        std::atomic<float>* grit = nullptr;
        std::atomic<float>* knock = nullptr;
        std::atomic<float>* thump = nullptr;
        std::atomic<float>* warmth = nullptr;
        std::atomic<float>* dust = nullptr;
        std::atomic<float>* wobble = nullptr;
        std::atomic<float>* era = nullptr;
        std::atomic<float>* pitch = nullptr;
        std::atomic<float>* lowCut = nullptr;
        std::atomic<float>* width = nullptr;
        std::atomic<float>* monoBass = nullptr;
        std::atomic<float>* sidechain = nullptr;
        std::atomic<float>* mix = nullptr;
        std::atomic<float>* output = nullptr;
        std::atomic<float>* bypass = nullptr;
    } raw;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchBoxProcessor)
};
