#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class BirchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BirchLookAndFeel();

    juce::Font graffitiFont (float height) const;
    static juce::Font uiFont (float height, bool bold = true);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& background,
                               bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

private:
    juce::Typeface::Ptr graffiti;
};

// Everything is laid out at one fixed size; the editor scales this view to fit the window.
class BirchView : public juce::Component,
                  public juce::FileDragAndDropTarget,
                  private juce::Timer
{
public:
    static constexpr int baseWidth = 1040, baseHeight = 740;

    BirchView (BirchBoxProcessor&, BirchLookAndFeel&);
    ~BirchView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::String name;
        juce::Rectangle<int> nameArea;
        bool big = true;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        juce::Rectangle<int> area;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& name, const juce::String& tooltip, bool big);
    void setupToggle (juce::TextButton&, const juce::String& text, int radioGroup);
    void setupParamToggle (juce::TextButton&, const juce::String& text, const juce::String& paramId,
                           std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>&, const juce::String& tooltip);
    void togglePitch33();
    void chooseFile();
    void loadFile (const juce::File&);
    void syncControls();
    void timerCallback() override;

    void renderBackground();
    void paintSection (juce::Graphics&, const Section&, bool withTitle);
    void paintMeter (juce::Graphics&, juce::Rectangle<int> area, float fill, float peak, bool gainReduction);

    BirchBoxProcessor& proc;
    BirchLookAndFeel& lnf;

    Knob grit, pitch, knock, thump, warmth, dust, wobble;
    Knob lowCut, width, mix, output;

    juce::ComboBox presetBox;
    juce::TextButton presetPrev, presetNext;

    juce::TextButton eraButtons[3];
    std::unique_ptr<juce::ParameterAttachment> eraAttachment;

    juce::TextButton bypassButton, monoBassButton, sidechainButton, pitch33Button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment, monoBassAttachment, sidechainAttachment;

    juce::TextButton sourceInput, sourceDemo, sourceFile;
    juce::Label fileLabel;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::TooltipWindow tooltips { this, 500 };

    juce::Image background, logo;
    Section header { "", {} }, sampler { "SAMPLER", {} }, punch { "PUNCH", {} }, tape { "TAPE & VINYL", {} },
            toneStereo { "TONE & STEREO", {} }, outputSection { "OUTPUT", {} }, sourceSection { "AVOTS", {} };
    juce::Rectangle<int> logoArea, titleArea, outMeterArea, grMeterArea;

    float meterLevel = 0.0f, peakLevel = 0.0f, grLevel = 0.0f;
    int peakHold = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchView)
};

class BirchBoxEditor : public juce::AudioProcessorEditor
{
public:
    explicit BirchBoxEditor (BirchBoxProcessor&);
    ~BirchBoxEditor() override;

    void resized() override;

private:
    BirchLookAndFeel lnf;
    BirchView view;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BirchBoxEditor)
};
