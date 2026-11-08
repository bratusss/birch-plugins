#pragma once

// Birch visual language (gold foil on black, after the Birch Please logo), shared with Birch Box

#include <juce_gui_basics/juce_gui_basics.h>

// JUCE treats plain char literals as ASCII, so Latvian text goes through this
inline juce::String lv (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace colours
{
    inline const juce::Colour ink        { 0xff000000 };
    inline const juce::Colour surface    { 0xff16120d };
    inline const juce::Colour lcd        { 0xff050403 };
    inline const juce::Colour track      { 0xff2b241a };
    inline const juce::Colour goldLight  { 0xffffd76e };
    inline const juce::Colour gold       { 0xfff2b13a };
    inline const juce::Colour goldDark   { 0xffc27f17 };
    inline const juce::Colour shine      { 0xfffff3c4 };
    inline const juce::Colour text       { 0xfff7e2ad };
    inline const juce::Colour dimText    { 0xff9a8150 };
    inline const juce::Colour danger     { 0xffe0532c };
}

namespace birchui
{
    void fillGold (juce::Graphics&, const juce::Path&, float stripeAlpha = 0.6f);
    juce::Path textPath (const juce::Font&, const juce::String&, juce::Rectangle<float>, juce::Justification);
    void drawGraffiti (juce::Graphics&, const juce::Font&, const juce::String& text, juce::Rectangle<float>,
                       juce::Justification, float outline, bool shadow = false);
    void drawGoldBar (juce::Graphics&, juce::Rectangle<float>);
    void drawScrew (juce::Graphics&, juce::Point<float> centre, float angle);
    void glowLine (juce::Graphics&, const juce::Path&, juce::Colour, float width);
}

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
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;

private:
    juce::Typeface::Ptr graffiti;
};
