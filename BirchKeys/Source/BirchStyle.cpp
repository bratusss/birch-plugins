#include "BirchStyle.h"
#include "BinaryData.h"



namespace birchui
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // Gold gradient with the logo's double diagonal shine stripes
    void fillGold (juce::Graphics& g, const juce::Path& path, float stripeAlpha)
    {
        const auto b = path.getBounds();
        juce::ColourGradient gradient (colours::goldLight, 0.0f, b.getY(), colours::goldDark, 0.0f, b.getBottom(), false);
        gradient.addColour (0.45, colours::gold);
        g.setGradientFill (gradient);
        g.fillPath (path);

        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (path);
        g.setColour (colours::shine.withAlpha (stripeAlpha));
        const float h = b.getHeight();
        const float spacing = juce::jmax (10.0f, h * 0.42f);
        const float thick = juce::jmax (1.2f, h * 0.035f);
        for (float x = b.getX() - h; x < b.getRight() + h; x += spacing)
        {
            g.drawLine (x, b.getBottom(), x + h * 0.55f, b.getY(), thick);
            g.drawLine (x + thick * 2.6f, b.getBottom(), x + thick * 2.6f + h * 0.55f, b.getY(), thick);
        }
    }

    juce::Path textPath (const juce::Font& font, const juce::String& text, juce::Rectangle<float> area, juce::Justification just)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addFittedText (font, text, area.getX(), area.getY(), area.getWidth(), area.getHeight(), just, 1);
        juce::Path path;
        glyphs.createPath (path);
        return path;
    }

    // Logo-style lettering: thick black outline, gold foil fill, thin inner line
    void drawGraffiti (juce::Graphics& g, const juce::Font& font, const juce::String& text,
                       juce::Rectangle<float> area, juce::Justification just, float outline, bool shadow)
    {
        const auto path = textPath (font, text, area, just);
        if (path.isEmpty())
            return;

        juce::Path outlined;
        juce::PathStrokeType (outline, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (outlined, path);
        outlined.addPath (path);

        if (shadow)
            juce::DropShadow (juce::Colours::black.withAlpha (0.9f), (int) (outline * 2.5f), { 0, (int) (outline * 0.8f) }).drawForPath (g, outlined);

        g.setColour (colours::ink);
        g.fillPath (outlined);
        fillGold (g, path);
        g.setColour (colours::ink.withAlpha (0.85f));
        g.strokePath (path, juce::PathStrokeType (juce::jmax (1.0f, outline * 0.15f)));
    }

    // The bar between the two words in the logo: a long bar with a small block at each end
    void drawGoldBar (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const float cap = r.getHeight() * 1.4f;
        const float gap = r.getHeight() * 0.35f;
        juce::Path bar;
        bar.addRoundedRectangle (r.getX(), r.getY(), cap, r.getHeight(), 2.0f);
        bar.addRoundedRectangle (r.getX() + cap + gap, r.getY(), r.getWidth() - 2.0f * (cap + gap), r.getHeight(), 3.0f);
        bar.addRoundedRectangle (r.getRight() - cap, r.getY(), cap, r.getHeight(), 2.0f);
        g.setColour (colours::ink);
        g.strokePath (bar, juce::PathStrokeType (4.0f));
        fillGold (g, bar);
        g.setColour (colours::ink);
        g.strokePath (bar, juce::PathStrokeType (1.2f));
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> c, float angle)
    {
        const auto r = juce::Rectangle<float> (10.0f, 10.0f).withCentre (c);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillEllipse (r.translated (0.0f, 1.5f).expanded (1.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a7550), r.getX(), r.getY(),
                                                 juce::Colour (0xff2a2116), r.getRight(), r.getBottom(), false));
        g.fillEllipse (r);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (3.6f, angle), c.getPointOnCircumference (3.6f, angle + juce::MathConstants<float>::pi)), 1.6f);
    }

    void glowLine (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float width)
    {
        for (auto [extra, alpha] : { std::pair { 12.0f, 0.05f }, { 7.0f, 0.09f }, { 3.5f, 0.16f } })
        {
            g.setColour (colour.withAlpha (alpha));
            g.strokePath (path, juce::PathStrokeType (width + extra, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

using namespace birchui;

//==============================================================================
BirchLookAndFeel::BirchLookAndFeel()
    : graffiti (juce::Typeface::createSystemTypefaceFor (BinaryData::SedgwickAveDisplayRegular_ttf,
                                                         BinaryData::SedgwickAveDisplayRegular_ttfSize))
{
    setColour (juce::Slider::textBoxTextColourId, colours::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, colours::gold.withAlpha (0.4f));
    setColour (juce::Label::textColourId, colours::text);
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::textColourId, colours::text);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::highlightColourId, colours::gold.withAlpha (0.4f));
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::CaretComponent::caretColourId, colours::gold);
    setColour (juce::TextButton::buttonColourId, colours::surface);
    setColour (juce::TextButton::buttonOnColourId, colours::gold);
    setColour (juce::TextButton::textColourOffId, colours::gold);
    setColour (juce::TextButton::textColourOnId, colours::ink);
    setColour (juce::ComboBox::textColourId, colours::goldLight);
    setColour (juce::ComboBox::arrowColourId, colours::gold);
    setColour (juce::PopupMenu::backgroundColourId, colours::surface);
    setColour (juce::PopupMenu::textColourId, colours::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::gold);
    setColour (juce::PopupMenu::highlightedTextColourId, colours::ink);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xf0100d09));
    setColour (juce::TooltipWindow::textColourId, colours::text);
    setColour (juce::TooltipWindow::outlineColourId, colours::goldDark);
}

juce::Font BirchLookAndFeel::graffitiFont (float height) const
{
    return juce::Font (juce::FontOptions().withTypeface (graffiti).withHeight (height));
}

juce::Font BirchLookAndFeel::uiFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

void BirchLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const bool big = slider.getProperties().getWithDefault ("big", true);
    const bool hot = slider.isMouseOverOrDragging();
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    const float arcWidth = big ? 4.5f : 3.5f;
    const float arcRadius = radius - 9.0f;
    const float ringRadius = arcRadius - (big ? 10.0f : 8.0f);
    const float faceRadius = ringRadius * 0.72f;

    // scale ticks, lit up to the current value
    const int ticks = big ? 11 : 7;
    for (int i = 0; i < ticks; ++i)
    {
        const float t = (float) i / (float) (ticks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const bool lit = t <= sliderPos + 0.001f;
        const float inner = radius - ((i % 5 == 0 || ! big) ? 5.5f : 3.5f);
        g.setColour (lit ? colours::goldLight.withAlpha (0.9f) : colours::dimText.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (inner, a), centre.getPointOnCircumference (radius, a)), 1.6f);
    }

    juce::Path trackArc;
    trackArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.strokePath (trackArc, juce::PathStrokeType (arcWidth + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (sliderPos > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, startAngle, angle, true);
        glowLine (g, valueArc, colours::gold, arcWidth);
        juce::Path valueShape;
        juce::PathStrokeType (arcWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded).createStrokedPath (valueShape, valueArc);
        fillGold (g, valueShape, 0.3f);
    }

    // drop shadow under the cap
    const auto ring = juce::Rectangle<float> (ringRadius * 2.0f, ringRadius * 2.0f).withCentre (centre);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.8f), centre.x, centre.y + 5.0f,
                                             juce::Colours::transparentBlack, centre.x + ringRadius * 1.25f, centre.y + 5.0f, true));
    g.fillEllipse (ring.expanded (ringRadius * 0.25f).translated (0.0f, 5.0f));

    // knurled black skirt
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3226), centre.x, ring.getY(),
                                             juce::Colour (0xff060504), centre.x, ring.getBottom(), false));
    g.fillEllipse (ring);
    const int ridges = big ? 64 : 48;
    for (int i = 0; i < ridges; ++i)
    {
        const float a = twoPi * (float) i / (float) ridges;
        g.setColour (i % 2 == 0 ? juce::Colours::white.withAlpha (0.07f) : juce::Colours::black.withAlpha (0.55f));
        g.drawLine (juce::Line<float> (centre.getPointOnCircumference (ringRadius * 0.8f, a),
                                       centre.getPointOnCircumference (ringRadius * 0.98f, a)), 1.4f);
    }
    g.setColour ((hot ? colours::goldLight : colours::goldDark).withAlpha (hot ? 0.9f : 0.6f));
    g.drawEllipse (ring, 1.3f);

    // gold foil face with a soft top-left light
    juce::Path face;
    face.addEllipse (juce::Rectangle<float> (faceRadius * 2.0f, faceRadius * 2.0f).withCentre (centre));
    g.setColour (juce::Colours::black);
    g.strokePath (face, juce::PathStrokeType (3.0f));
    fillGold (g, face, 0.45f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.35f), centre.x - faceRadius * 0.45f, centre.y - faceRadius * 0.55f,
                                             juce::Colours::transparentWhite, centre.x + faceRadius * 0.4f, centre.y + faceRadius * 0.4f, true));
    g.fillPath (face);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, centre.x, centre.y - faceRadius * 0.2f,
                                             juce::Colours::black.withAlpha (0.35f), centre.x, centre.y + faceRadius, false));
    g.fillPath (face);
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.strokePath (face, juce::PathStrokeType (1.2f));

    // pointer notch cut into the face
    const auto tip = centre.getPointOnCircumference (faceRadius * 0.9f, angle);
    const auto tail = centre.getPointOnCircumference (faceRadius * 0.18f, angle);
    juce::Path pointer;
    pointer.startNewSubPath (tail);
    pointer.lineTo (tip);
    g.setColour (juce::Colours::black);
    g.strokePath (pointer, juce::PathStrokeType (big ? 5.0f : 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (colours::shine.withAlpha (0.5f));
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (centre.getPointOnCircumference (ringRadius * 0.89f, angle)));
}

void BirchLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                             bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.5f);
    if (down)
        r = r.translated (0.0f, 1.0f);
    const float corner = juce::jmin (8.0f, r.getHeight() * 0.3f);
    juce::Path shape;
    shape.addRoundedRectangle (r, corner);

    const bool on = button.getToggleState();
    const bool led = button.getProperties().getWithDefault ("led", false);

    if (on && ! led)
    {
        glowLine (g, shape, colours::gold, 0.5f);
        fillGold (g, shape, 0.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.3f), 0.0f, r.getY(),
                                                 juce::Colours::transparentWhite, 0.0f, r.getCentreY(), false));
        g.fillRoundedRectangle (r.reduced (2.0f).withHeight (r.getHeight() * 0.45f), corner);
        g.setColour (colours::ink);
        g.strokePath (shape, juce::PathStrokeType (2.0f));
        return;
    }

    g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff0e0b08 : 0xff221c15), 0.0f, r.getY(),
                                             juce::Colour (0xff0b0907), 0.0f, r.getBottom(), false));
    g.fillPath (shape);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawLine (r.getX() + corner, r.getY() + 1.0f, r.getRight() - corner, r.getY() + 1.0f, 1.0f);
    g.setColour ((highlighted ? colours::goldLight : colours::goldDark).withAlpha (highlighted ? 0.95f : 0.55f));
    g.strokePath (shape, juce::PathStrokeType (1.4f));

    if (led)
    {
        const auto ledColour = button.findColour (juce::TextButton::buttonOnColourId);
        const auto c = juce::Point<float> (r.getX() + 17.0f, r.getCentreY());
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (ledColour.withAlpha (0.55f), c.x, c.y, ledColour.withAlpha (0.0f), c.x + 14.0f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (28.0f, 28.0f).withCentre (c));
        }
        const auto dot = juce::Rectangle<float> (9.0f, 9.0f).withCentre (c);
        g.setColour (on ? ledColour.brighter (0.4f) : juce::Colour (0xff2a2218));
        g.fillEllipse (dot);
        g.setColour (juce::Colours::black);
        g.drawEllipse (dot, 1.2f);
        if (on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.7f));
            g.fillEllipse (dot.reduced (2.6f).translated (-0.8f, -0.8f));
        }
    }
}

void BirchLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool down)
{
    auto area = button.getLocalBounds().toFloat();
    if (down)
        area = area.translated (0.0f, 1.0f);

    const int arrow = button.getProperties().getWithDefault ("arrow", 0);
    if (arrow != 0)
    {
        const auto c = area.getCentre();
        juce::Path tri;
        if (arrow < 0)
            tri.addTriangle (c.x - 5.0f, c.y, c.x + 4.0f, c.y - 6.0f, c.x + 4.0f, c.y + 6.0f);
        else
            tri.addTriangle (c.x + 5.0f, c.y, c.x - 4.0f, c.y - 6.0f, c.x - 4.0f, c.y + 6.0f);
        g.setColour (button.isOver() ? colours::goldLight : colours::gold);
        g.fillPath (tri);
        return;
    }

    const bool on = button.getToggleState();
    const bool led = button.getProperties().getWithDefault ("led", false);
    if (led)
    {
        area.removeFromLeft (26.0f);
        g.setColour (on ? colours::goldLight : colours::dimText);
    }
    else
    {
        g.setColour (on ? colours::ink : (button.isOver() ? colours::goldLight : colours::gold));
    }

    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.drawFittedText (button.getButtonText(), area.reduced (6.0f, 2.0f).toNearestInt(),
                      led ? juce::Justification::centredLeft : juce::Justification::centred, 1);
}

juce::Font BirchLookAndFeel::getTextButtonFont (juce::TextButton& button, int buttonHeight)
{
    if (button.getProperties().getWithDefault ("graffiti", false))
        return graffitiFont ((float) buttonHeight * 0.72f);
    return uiFont (juce::jmin (14.0f, (float) buttonHeight * 0.42f)).withExtraKerningFactor (0.06f);
}

void BirchLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                     int, int, int, int, juce::ComboBox& box)
{
    // inset LCD
    const auto r = juce::Rectangle<int> (width, height).toFloat().reduced (1.0f);
    g.setColour (colours::lcd);
    g.fillRoundedRectangle (r, 6.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.9f), 0.0f, r.getY(),
                                             juce::Colours::transparentBlack, 0.0f, r.getY() + 10.0f, false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.10f), r.getCentreX(), r.getCentreY(),
                                             juce::Colours::transparentBlack, r.getX(), r.getCentreY(), true));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour ((box.isMouseOver (true) ? colours::goldLight : colours::goldDark).withAlpha (0.7f));
    g.drawRoundedRectangle (r, 6.0f, 1.3f);

    const float ax = (float) width - 22.0f, ay = (float) height * 0.5f;
    g.setColour (colours::goldDark.withAlpha (0.4f));
    g.drawLine (ax - 10.0f, r.getY() + 7.0f, ax - 10.0f, r.getBottom() - 7.0f, 1.0f);
    juce::Path arrow;
    arrow.addTriangle (ax, ay - 3.0f, ax + 10.0f, ay - 3.0f, ax + 5.0f, ay + 4.0f);
    g.setColour (colours::gold);
    g.fillPath (arrow);
}

void BirchLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (14, 1, box.getWidth() - 50, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font BirchLookAndFeel::getComboBoxFont (juce::ComboBox&) { return graffitiFont (25.0f); }
juce::Font BirchLookAndFeel::getPopupMenuFont() { return uiFont (16.0f); }

void BirchLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (colours::surface);
    g.setColour (colours::goldDark);
    g.drawRect (0, 0, width, height, 1);
}

void BirchLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    // slider value read-outs become small black pills
    if (dynamic_cast<juce::Slider*> (label.getParentComponent()) != nullptr)
    {
        const auto r = label.getLocalBounds().toFloat().reduced (8.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (colours::goldDark.withAlpha (0.45f));
        g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);

        if (! label.isBeingEdited())
        {
            g.setColour (colours::text);
            g.setFont (uiFont (12.5f));
            g.drawText (label.getText(), r, juce::Justification::centred, false);
        }
        return;
    }

    LookAndFeel_V4::drawLabel (g, label);
}

// Organ drawbars: a rod in a slot with a numbered cap; gold caps for the octave footages
void BirchLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                         float minPos, float maxPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (! (bool) slider.getProperties().getWithDefault ("drawbar", false))
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minPos, maxPos, style, slider);
        return;
    }

    const auto r = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto slot = r.withSizeKeepingCentre (12.0f, r.getHeight() + 10.0f);
    g.setColour (colours::lcd);
    g.fillRoundedRectangle (slot, 6.0f);
    g.setColour (colours::goldDark.withAlpha (0.5f));
    g.drawRoundedRectangle (slot, 6.0f, 1.0f);

    const float capHeight = 28.0f;
    const auto rod = juce::Rectangle<float> (r.getCentreX() - 4.0f, sliderPos, 8.0f, r.getBottom() + 5.0f - sliderPos);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6d6455), rod.getX(), 0.0f, juce::Colour (0xff1d1a15), rod.getRight(), 0.0f, false));
    g.fillRect (rod);

    const auto cap = juce::Rectangle<float> (r.getWidth() - 4.0f, capHeight).withCentre ({ r.getCentreX(), sliderPos });
    juce::Path capShape;
    capShape.addRoundedRectangle (cap, 5.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (cap.translated (0.0f, 3.0f), 5.0f);
    const bool octave = slider.getProperties().getWithDefault ("octave", false);
    if (octave)
        birchui::fillGold (g, capShape, 0.4f);
    else
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3027), 0.0f, cap.getY(), juce::Colour (0xff0b0907), 0.0f, cap.getBottom(), false));
        g.fillPath (capShape);
    }
    g.setColour (juce::Colours::black);
    g.strokePath (capShape, juce::PathStrokeType (1.5f));
    g.setColour (octave ? colours::ink : colours::goldLight);
    g.setFont (uiFont (14.0f));
    g.drawText (juce::String (juce::roundToInt (slider.getValue())), cap, juce::Justification::centred);
}

