#pragma once

#include "BirchStyle.h"
#include "PluginProcessor.h"

// On-screen keyboard with ivory keys and gold pressed keys
class BirchKeyboard : public juce::MidiKeyboardComponent, public juce::SettableTooltipClient
{
public:
    explicit BirchKeyboard (BirchKeysProcessor&);

private:
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
};

class BirchKeysView : public juce::Component, private juce::Timer
{
public:
    static constexpr int baseWidth = 1180, baseHeight = 862;

    BirchKeysView (BirchKeysProcessor&, BirchLookAndFeel&);
    ~BirchKeysView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::String name;
        juce::Rectangle<int> nameArea;
        bool big = true;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Stepper
    {
        juce::TextButton down, up;
        juce::Label label;
        juce::String paramId, prefix;
    };

    struct Radio
    {
        std::vector<std::unique_ptr<juce::TextButton>> buttons;
        std::unique_ptr<juce::ParameterAttachment> attachment;
    };

    enum class Page { modal, string, organ, analog, fm };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& tooltip, bool big);
    void setupStepper (Stepper&, const juce::String& paramId, const juce::String& prefix, const juce::String& tooltip);
    void setupRadio (Radio&, const juce::StringArray& names, const juce::String& paramId, int group,
                     const juce::String& tooltip, bool graffiti);
    void stepParam (const juce::String& id, int delta);

    void selectCategory (const juce::String& category);
    void rebuildInstrumentList();
    void rebuildPresetList();
    void savePreset();
    void syncInstrument();
    void showPage (Page);
    void timerCallback() override;

    void renderBackground();
    void paintSection (juce::Graphics&, juce::Rectangle<int> area, const juce::String& title, bool baked = true);

    std::vector<Knob*> soundKnobs();
    std::vector<Knob*> pageKnobs (Page);

    BirchKeysProcessor& proc;
    BirchLookAndFeel& lnf;

    // header
    juce::StringArray categories;
    std::vector<std::unique_ptr<juce::TextButton>> categoryTabs;
    juce::ComboBox presetBox;
    juce::TextButton presetPrev, presetNext, saveButton;
    juce::Array<juce::File> userPresetFiles;

    // sound
    juce::ComboBox instrumentBox;
    std::vector<int> instrumentIds; // instruments listed in the current category
    Knob level, tone, decay, release, character, motion, rate, velocity, width, attack, glide, tune;
    juce::TextButton monoButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttachment;
    Stepper octave, bend;

    // engine pages
    Knob bell, noise, detune, stretch, pickupOffset;
    Knob pluck, body, finger, pickup, drop, mwah;
    juce::Slider drawbars[9];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> drawbarAttachments[9];
    Radio percHarmonic;
    Knob percDecay, click, chiff;
    Radio osc1Wave, osc2Wave;
    Knob osc2Level, osc2Semis, osc2Cents, sub, noiseLevel, breath, cutoff, envAmount, filterDecay, sustain, drive, unisonSpread;
    Stepper unison;
    Knob fmRatio[3], fmIndex[3], fmDecay[3];
    juce::Label engineInfo;

    // sampler chain
    Knob grit, filter, reso, warmth, wobble, dust, room, volume;
    Radio era;

    BirchKeyboard keyboard;
    juce::TooltipWindow tooltips { this, 500 };

    juce::Image background, logo;
    juce::Rectangle<int> logoArea, titleArea, headerArea, soundArea, engineArea, fxArea, keyboardArea, meterArea;
    juce::String engineTitle, currentCategory;
    Page page = Page::modal;
    int shownInstrument = -1, shownPreset = -2, presetCount = -1;
    juce::String shownPresetName;
    float meterLevel = 0.0f, peakLevel = 0.0f;
    int peakHold = 0;
    bool focusGrabbed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchKeysView)
};

class BirchKeysEditor : public juce::AudioProcessorEditor
{
public:
    explicit BirchKeysEditor (BirchKeysProcessor&);
    ~BirchKeysEditor() override;

    void resized() override;

private:
    BirchLookAndFeel lnf;
    BirchKeysView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchKeysEditor)
};
