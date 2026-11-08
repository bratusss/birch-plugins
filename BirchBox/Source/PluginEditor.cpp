#include "PluginEditor.h"
#include "BinaryData.h"

// JUCE treats plain char literals as ASCII, so Latvian text goes through this
static juce::String lv (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace colours
{
    // Gold foil on black, after the Birch Please logo
    const juce::Colour ink        { 0xff000000 };
    const juce::Colour surface    { 0xff16120d };
    const juce::Colour lcd        { 0xff050403 };
    const juce::Colour track      { 0xff2b241a };
    const juce::Colour goldLight  { 0xffffd76e };
    const juce::Colour gold       { 0xfff2b13a };
    const juce::Colour goldDark   { 0xffc27f17 };
    const juce::Colour shine      { 0xfffff3c4 };
    const juce::Colour text       { 0xfff7e2ad };
    const juce::Colour dimText    { 0xff9a8150 };
    const juce::Colour danger     { 0xffe0532c };
}

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    // Gold gradient with the logo's double diagonal shine stripes
    void fillGold (juce::Graphics& g, const juce::Path& path, float stripeAlpha = 0.6f)
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
                       juce::Rectangle<float> area, juce::Justification just, float outline, bool shadow = false)
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

//==============================================================================
BirchView::BirchView (BirchBoxProcessor& p, BirchLookAndFeel& l)
    : proc (p), lnf (l)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::birch_logo_png, BinaryData::birch_logo_pngSize);

    setupKnob (grit, "grit", "GRIT", lv ("Semplera raksturs: samazina bitu dziļumu un sample rate (12-bitu skaņa)."), true);
    setupKnob (pitch, "pitch", "PITCH", lv ("Maina skaņas augstumu pustoņos, tempu nemainot. Uz leju = smagākas, tumšākas bungas."), true);
    setupKnob (knock, "knock", "KNOCK", lv ("Transientu pastiprinātājs + kompresors: kick un snare izlec priekšā."), true);
    setupKnob (thump, "thump", "THUMP", lv ("Sapiesātina tikai zemās frekvences (zem 110 Hz): biezāks kick."), true);
    setupKnob (warmth, "warmth", "WARMTH", lv ("Lentes saturācija visam signālam: noapaļo pīķus, pieliek siltumu un salīmē bungas kopā."), true);
    setupKnob (dust, "dust", "DUST", lv ("Vinila troksnis un sprakšķi. Bungu sitienos troksnis automātiski noklust."), true);
    setupKnob (wobble, "wobble", "WOBBLE", lv ("Lentes/plašu wow & flutter: viegla skaņas augstuma ļodzīšanās."), true);

    setupKnob (lowCut, "lowcut", "LOW CUT", lv ("Nogriež dūkoņu un subbasu pirms apstrādes, lai Thump nepūš dubļus."), false);
    setupKnob (width, "width", "WIDTH", lv ("Stereo platums: 0 % = mono, 100 % = oriģināls, 200 % = ļoti plats."), false);
    setupKnob (mix, "mix", "MIX", lv ("Cik daudz apstrādātā signāla (100 % = tikai efekts, mazāk = paralēlā apstrāde)."), false);
    setupKnob (output, "output", "OUT", lv ("Izejas skaļums."), false);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setTooltip (lv ("Gatavi boom bap uzstādījumi."));
    presetBox.onChange = [this]
    {
        if (presetBox.getSelectedId() > 0)
            proc.setCurrentProgram (presetBox.getSelectedId() - 1);
    };
    addAndMakeVisible (presetBox);

    presetPrev.getProperties().set ("arrow", -1);
    presetNext.getProperties().set ("arrow", 1);
    auto step = [this] (int delta)
    {
        const int count = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + delta + count) % count);
        syncControls();
    };
    presetPrev.onClick = [step] { step (-1); };
    presetNext.onClick = [step] { step (1); };
    addAndMakeVisible (presetPrev);
    addAndMakeVisible (presetNext);

    const char* eraNames[] = { "'88", "'94", "'98" };
    for (int i = 0; i < 3; ++i)
    {
        setupToggle (eraButtons[i], eraNames[i], 1001);
        eraButtons[i].getProperties().set ("graffiti", true);
        eraButtons[i].setTooltip (lv ("Era: maina visu pogu raksturu ('88 netīrākais, '98 tīrākais un smagākais)."));
        eraButtons[i].onClick = [this, i] { eraAttachment->setValueAsCompleteGesture ((float) i); };
    }
    eraAttachment = std::make_unique<juce::ParameterAttachment> (
        *proc.apvts.getParameter ("era"),
        [this] (float value)
        {
            const int index = juce::roundToInt (value);
            for (int i = 0; i < 3; ++i)
                eraButtons[i].setToggleState (i == index, juce::dontSendNotification);
        });
    eraAttachment->sendInitialUpdate();

    setupParamToggle (bypassButton, "BYPASS", "bypass", bypassAttachment,
                      lv ("Izslēdz efektu, lai salīdzinātu ar oriģinālu (A/B)."));
    bypassButton.setColour (juce::TextButton::buttonOnColourId, colours::danger);
    setupParamToggle (monoBassButton, "MONO BASS", "monobass", monoBassAttachment,
                      lv ("Zemās frekvences (zem 120 Hz) savelk mono. Kick un bass paliek centrā un skan stingrāk."));
    setupParamToggle (sidechainButton, "SIDECHAIN", "sidechain", sidechainAttachment,
                      lv ("Knock kompresors un Dust reaģē uz sidechain ieeju (piem., atsevišķu kick celiņu). "
                          "Demo bīta režīmā sidechain ir demo kick."));
    for (auto* b : { &bypassButton, &monoBassButton, &sidechainButton })
        b->getProperties().set ("led", true);

    pitch33Button.setButtonText (lv ("45→33"));
    pitch33Button.setTooltip (lv ("Klasiskais triks: 45 apgr./min plate atskaņota ar 33 (-5,2 pustoņi). Vēlreiz nospiežot, atgriež 0."));
    pitch33Button.getProperties().set ("graffiti", true);
    pitch33Button.onClick = [this] { togglePitch33(); };
    addAndMakeVisible (pitch33Button);

    setupToggle (sourceInput, "IEEJA", 1002);
    setupToggle (sourceDemo, lv ("DEMO BĪTS"), 1002);
    setupToggle (sourceFile, "FAILS...", 1002);
    sourceInput.setTooltip (lv ("Apstrādā signālu no DAW vai audio ieejas."));
    sourceDemo.setTooltip (lv ("Iebūvēts 90 BPM boom bap bīts testēšanai."));
    sourceFile.setTooltip (lv ("Izvēlies WAV/MP3/FLAC loop. Failu var arī vienkārši ievilkt logā."));
    sourceInput.onClick = [this] { proc.setSource (BirchBoxProcessor::Source::input); };
    sourceDemo.onClick = [this] { proc.setSource (BirchBoxProcessor::Source::demoBeat); };
    sourceFile.onClick = [this] { chooseFile(); };

    fileLabel.setColour (juce::Label::textColourId, colours::dimText);
    fileLabel.setJustificationType (juce::Justification::centred);
    fileLabel.setFont (BirchLookAndFeel::uiFont (12.0f, false));
    addAndMakeVisible (fileLabel);

    syncControls();
    setSize (baseWidth, baseHeight);
    startTimerHz (30);
}

BirchView::~BirchView()
{
    stopTimer();
}

void BirchView::setupKnob (Knob& knob, const juce::String& paramId, const juce::String& name,
                           const juce::String& tooltip, bool big)
{
    knob.name = name;
    knob.big = big;
    knob.slider.getProperties().set ("big", big);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 96, 22);
    knob.slider.setTooltip (tooltip);
    knob.slider.setDoubleClickReturnValue (true, (double) proc.apvts.getParameter (paramId)->convertFrom0to1 (
                                                     proc.apvts.getParameter (paramId)->getDefaultValue()));
    addAndMakeVisible (knob.slider);
    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, knob.slider);
}

void BirchView::setupToggle (juce::TextButton& button, const juce::String& text, int radioGroup)
{
    button.setButtonText (text);
    button.setClickingTogglesState (true);
    button.setRadioGroupId (radioGroup, juce::dontSendNotification);
    addAndMakeVisible (button);
}

void BirchView::setupParamToggle (juce::TextButton& button, const juce::String& text, const juce::String& paramId,
                                  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>& attachment,
                                  const juce::String& tooltip)
{
    button.setButtonText (text);
    button.setClickingTogglesState (true);
    button.setTooltip (tooltip);
    addAndMakeVisible (button);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, paramId, button);
}

void BirchView::togglePitch33()
{
    auto* param = proc.apvts.getParameter ("pitch");
    const bool isOn = std::abs (param->convertFrom0to1 (param->getValue()) - birch::rpm45to33Semitones) < 0.05f;
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 (isOn ? 0.0f : birch::rpm45to33Semitones));
    param->endChangeGesture();
    syncControls();
}

void BirchView::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> (lv ("Izvēlies bungu loop"), juce::File(),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file.existsAsFile())
                                  loadFile (file);
                              chooser.reset();
                              syncControls();
                          });
}

void BirchView::loadFile (const juce::File& file)
{
    if (proc.loadLoopFile (file))
        proc.setSource (BirchBoxProcessor::Source::file);
    else
        fileLabel.setText (lv ("Neizdevās nolasīt: ") + file.getFileName(), juce::dontSendNotification);
    syncControls();
}

void BirchView::syncControls()
{
    const auto source = proc.getSource();
    sourceInput.setToggleState (source == BirchBoxProcessor::Source::input, juce::dontSendNotification);
    sourceDemo.setToggleState (source == BirchBoxProcessor::Source::demoBeat, juce::dontSendNotification);
    sourceFile.setToggleState (source == BirchBoxProcessor::Source::file, juce::dontSendNotification);

    if (proc.hasLoopFile())
        fileLabel.setText (proc.getLoopFileName(), juce::dontSendNotification);
    else if (fileLabel.getText().isEmpty())
        fileLabel.setText (lv ("vai ievelc audio failu logā"), juce::dontSendNotification);

    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);

    auto* pitchParam = proc.apvts.getParameter ("pitch");
    const float semis = pitchParam->convertFrom0to1 (pitchParam->getValue());
    pitch33Button.setToggleState (std::abs (semis - birch::rpm45to33Semitones) < 0.05f, juce::dontSendNotification);
}

bool BirchView::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1;
}

void BirchView::filesDropped (const juce::StringArray& files, int, int)
{
    loadFile (juce::File (files[0]));
}

void BirchView::timerCallback()
{
    const float peak = proc.takeOutputPeak();
    meterLevel = juce::jmax (peak, meterLevel * 0.82f);
    if (peak >= peakLevel)
    {
        peakLevel = peak;
        peakHold = 45;
    }
    else if (--peakHold < 0)
    {
        peakLevel *= 0.92f;
    }
    grLevel = juce::jmax (proc.takeGainReduction(), grLevel * 0.85f);

    repaint (outMeterArea.expanded (36, 30));
    repaint (grMeterArea.expanded (36, 30));
    repaint (logoArea.expanded (40));

    if (! chooser)
        syncControls();

    // Dev aid: BIRCHBOX_SNAPSHOT=/path/file.png saves a picture of the UI once it has run a moment
    static int ticks = 0;
    const auto snapshotPath = juce::SystemStats::getEnvironmentVariable ("BIRCHBOX_SNAPSHOT", {});
    if (snapshotPath.isNotEmpty() && ++ticks == 45)
    {
        auto* target = getParentComponent() != nullptr ? getParentComponent() : this;
        juce::File file (snapshotPath);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat().writeImageToStream (target->createComponentSnapshot (target->getLocalBounds(), true, 1.0f), stream);
    }
}

//==============================================================================
void BirchView::paintSection (juce::Graphics& g, const Section& s, bool withTitle)
{
    const auto r = s.area.toFloat();
    juce::Path shape;
    shape.addRoundedRectangle (r, 14.0f);

    juce::DropShadow (juce::Colours::black.withAlpha (0.85f), 26, { 0, 10 }).drawForPath (g, shape);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xf41c1711), 0.0f, r.getY(),
                                             juce::Colour (0xf80a0806), 0.0f, r.getBottom(), false));
    g.fillPath (shape);

    // faint diagonal foil texture, like the logo's stripes but barely there
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (shape);
        g.setColour (colours::gold.withAlpha (0.018f));
        for (float x = r.getX() - r.getHeight(); x < r.getRight(); x += 26.0f)
            g.drawLine (x, r.getBottom(), x + r.getHeight() * 0.55f, r.getY(), 7.0f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.reduced (1.5f), 12.5f, 1.0f);
    g.setColour (colours::gold.withAlpha (0.42f));
    g.strokePath (shape, juce::PathStrokeType (1.2f));

    for (auto [corner, angle] : { std::pair { r.getTopLeft(), 0.6f }, { r.getTopRight(), 2.1f },
                                   { r.getBottomLeft(), 1.2f }, { r.getBottomRight(), 2.7f } })
        drawScrew (g, corner + juce::Point<float> (corner.x < r.getCentreX() ? 13.0f : -13.0f,
                                                   corner.y < r.getCentreY() ? 13.0f : -13.0f), angle);

    if (withTitle)
    {
        const auto font = lnf.graffitiFont (25.0f);
        const auto titleBox = juce::Rectangle<float> (r.getX() + 30.0f, r.getY() + 8.0f, r.getWidth() - 60.0f, 32.0f);
        drawGraffiti (g, font, s.title, titleBox, juce::Justification::centredLeft, 4.0f);

        const float textWidth = textPath (font, s.title, titleBox, juce::Justification::centredLeft).getBounds().getWidth();
        const float lineX = titleBox.getX() + textWidth + 12.0f;
        const float lineY = titleBox.getCentreY() + 1.0f;
        g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.55f), lineX, lineY,
                                                 colours::gold.withAlpha (0.0f), titleBox.getRight(), lineY, false));
        g.fillRect (juce::Rectangle<float> (lineX, lineY - 1.0f, titleBox.getRight() - lineX, 2.0f));
    }
}

void BirchView::renderBackground()
{
    constexpr int scale = 2;
    background = juce::Image (juce::Image::RGB, baseWidth * scale, baseHeight * scale, true);

    // brick wall
    {
        juce::Graphics g (background);
        g.addTransform (juce::AffineTransform::scale ((float) scale));
        g.fillAll (juce::Colour (0xff070605));

        juce::Random rnd (7);
        constexpr float brickW = 78.0f, brickH = 30.0f, mortar = 3.0f;
        for (int row = 0; (float) row * brickH < (float) baseHeight; ++row)
        {
            const float offset = (row % 2) ? brickW * 0.5f : 0.0f;
            for (float x = -offset; x < (float) baseWidth; x += brickW)
            {
                const juce::Rectangle<float> brick (x + mortar * 0.5f, (float) row * brickH + mortar * 0.5f,
                                                    brickW - mortar, brickH - mortar);
                const float shade = 0.09f + rnd.nextFloat() * 0.045f;
                const auto c = juce::Colour::fromFloatRGBA (shade * 1.18f, shade * 0.98f, shade * 0.8f, 1.0f);
                g.setGradientFill (juce::ColourGradient (c.brighter (0.25f), 0.0f, brick.getY(),
                                                         c.darker (0.45f), 0.0f, brick.getBottom(), false));
                g.fillRoundedRectangle (brick, 2.5f);
            }
        }
    }

    // grain
    {
        juce::Random rnd (99);
        juce::Image::BitmapData data (background, juce::Image::BitmapData::readWrite);
        for (int y = 0; y < data.height; ++y)
        {
            auto* line = data.getLinePointer (y);
            for (int x = 0; x < data.width; ++x)
            {
                auto* px = line + x * data.pixelStride;
                const int n = rnd.nextInt (17) - 8;
                for (int c = 0; c < 3; ++c)
                    px[c] = (juce::uint8) juce::jlimit (0, 255, px[c] + n);
            }
        }
    }

    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale ((float) scale));

    // vignette + overall darkening so the panels pop
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.25f), baseWidth * 0.5f, baseHeight * 0.4f,
                                             juce::Colours::black.withAlpha (0.92f), 0.0f, 0.0f, true));
    g.fillAll();

    // warm spray-paint haze behind the logo
    const auto logoCentre = logoArea.getCentre().toFloat();
    g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.14f), logoCentre.x, logoCentre.y,
                                             juce::Colours::transparentBlack, logoCentre.x + 330.0f, logoCentre.y, true));
    g.fillRect (getLocalBounds());

    // title: BIRCH / bar / BOX, like the logo
    {
        auto area = titleArea.toFloat();
        constexpr float barHeight = 12.0f;
        auto top = area.removeFromTop ((area.getHeight() - barHeight) * 0.5f);
        auto bar = area.removeFromTop (barHeight);
        auto bottom = area;
        drawGraffiti (g, lnf.graffitiFont (top.getHeight() * 1.22f), "BIRCH", top.expanded (0, 8), juce::Justification::centred, 8.0f, true);
        drawGoldBar (g, bar.reduced (0, 1));
        drawGraffiti (g, lnf.graffitiFont (bottom.getHeight() * 1.22f), "BOX", bottom.expanded (0, 8), juce::Justification::centred, 8.0f, true);

        g.setColour (colours::dimText);
        g.setFont (BirchLookAndFeel::uiFont (12.0f).withExtraKerningFactor (0.45f));
        g.drawText ("BOOM BAP DRUM BUS", titleArea.withY (titleArea.getBottom() + 6).withHeight (16), juce::Justification::centred);
    }

    paintSection (g, header, false);
    for (auto* s : { &sampler, &punch, &tape, &toneStereo, &outputSection, &sourceSection })
        paintSection (g, *s, true);

    g.setColour (colours::dimText);
    g.setFont (BirchLookAndFeel::uiFont (11.0f).withExtraKerningFactor (0.3f));
    g.drawText ("PRESET", presetBox.getBounds().translated (2, -20).withHeight (16), juce::Justification::centredLeft);
    g.drawText ("ERA", eraButtons[0].getBounds().translated (2, -20).withHeight (16), juce::Justification::centredLeft);

    for (auto* knob : { &grit, &pitch, &knock, &thump, &warmth, &dust, &wobble, &lowCut, &width, &mix, &output })
        drawGraffiti (g, lnf.graffitiFont (knob->big ? 28.0f : 22.0f), knob->name, knob->nameArea.toFloat(),
                      juce::Justification::centred, knob->big ? 5.0f : 4.0f);

    // meter scales
    g.setColour (colours::text.withAlpha (0.6f));
    g.setFont (BirchLookAndFeel::uiFont (10.5f));
    for (float db : { 0.0f, -6.0f, -12.0f, -24.0f, -48.0f })
    {
        const float y = (float) outMeterArea.getBottom() - 3.0f - ((db + 60.0f) / 60.0f) * ((float) outMeterArea.getHeight() - 6.0f);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> ((float) outMeterArea.getX() - 30.0f, y - 6.0f, 26.0f, 12.0f), juce::Justification::centredRight);
    }
    for (float db : { 0.0f, 6.0f, 12.0f, 18.0f, 24.0f })
    {
        const float y = (float) grMeterArea.getY() + 3.0f + (db / 24.0f) * ((float) grMeterArea.getHeight() - 6.0f);
        g.drawText (juce::String ((int) db), juce::Rectangle<float> ((float) grMeterArea.getX() - 30.0f, y - 6.0f, 26.0f, 12.0f), juce::Justification::centredRight);
    }
    for (auto [area, name] : { std::pair { outMeterArea, "OUT" }, { grMeterArea, "GR" } })
        drawGraffiti (g, lnf.graffitiFont (20.0f), name, area.toFloat().withY ((float) area.getBottom() + 3.0f).withHeight (24.0f).expanded (24.0f, 0.0f),
                      juce::Justification::centred, 4.0f);

    // logo, pre-scaled once for quality
    logo = logo.rescaled (logoArea.getWidth() * scale, logoArea.getHeight() * scale, juce::Graphics::highResamplingQuality);
}

void BirchView::paintMeter (juce::Graphics& g, juce::Rectangle<int> area, float fill, float peak, bool gainReduction)
{
    const auto r = area.toFloat();
    g.setColour (colours::lcd);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (colours::goldDark.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    constexpr int segments = 28;
    constexpr float gap = 2.0f;
    const auto inner = r.reduced (4.0f);
    const float segH = (inner.getHeight() - gap * (segments - 1)) / segments;

    for (int i = 0; i < segments; ++i)
    {
        const float t = ((float) i + 0.5f) / (float) segments;
        const float y = gainReduction ? inner.getY() + (float) i * (segH + gap)
                                      : inner.getBottom() - (float) (i + 1) * segH - (float) i * gap;
        const juce::Rectangle<float> seg (inner.getX(), y, inner.getWidth(), segH);

        juce::Colour c = gainReduction ? colours::danger.interpolatedWith (colours::gold, 1.0f - t)
                                       : (i >= segments - 3 ? colours::danger : i >= segments - 8 ? colours::goldLight : colours::gold);
        const bool lit = t <= fill;
        const bool isPeak = ! gainReduction && peak > 0.0f && std::abs (t - peak) < 0.5f / segments;

        if (lit || isPeak)
        {
            g.setColour (c.withAlpha (0.25f));
            g.fillRoundedRectangle (seg.expanded (2.0f, 1.5f), 2.0f);
            g.setColour (c);
        }
        else
        {
            g.setColour (c.withAlpha (0.07f));
        }
        g.fillRoundedRectangle (seg, 1.5f);
    }
}

void BirchView::paint (juce::Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    // logo breathes with the beat
    const float level = juce::jlimit (0.0f, 1.0f, meterLevel * 1.3f);
    const auto c = logoArea.getCentre().toFloat();
    g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.05f + 0.28f * level), c.x, c.y,
                                             juce::Colours::transparentBlack, c.x + logoArea.getWidth() * 0.62f, c.y, true));
    g.fillEllipse (logoArea.toFloat().expanded (40.0f));
    g.drawImage (logo, logoArea.toFloat());

    const float outDb = juce::Decibels::gainToDecibels (meterLevel, -60.0f);
    const float peakDb = juce::Decibels::gainToDecibels (peakLevel, -60.0f);
    paintMeter (g, outMeterArea, (outDb + 60.0f) / 60.0f, (peakDb + 60.0f) / 60.0f, false);
    paintMeter (g, grMeterArea, juce::jlimit (0.0f, 1.0f, grLevel / 24.0f), 0.0f, true);

    g.setColour (colours::text);
    g.setFont (BirchLookAndFeel::uiFont (11.0f));
    g.drawText (outDb <= -59.9f ? juce::String ("-inf") : juce::String (outDb, 1), outMeterArea.withY (outMeterArea.getY() - 18).withHeight (14).expanded (20, 0), juce::Justification::centred);
    g.drawText ("-" + juce::String (grLevel, 1), grMeterArea.withY (grMeterArea.getY() - 18).withHeight (14).expanded (20, 0), juce::Justification::centred);
}

void BirchView::resized()
{
    logoArea = { 26, 14, 202, 189 };
    titleArea = { 246, 34, 300, 136 };

    header.area = { 568, 22, 448, 174 };
    presetBox.setBounds (592, 58, 298, 42);
    presetPrev.setBounds (898, 58, 42, 42);
    presetNext.setBounds (946, 58, 42, 42);
    for (int i = 0; i < 3; ++i)
        eraButtons[i].setBounds (592 + i * 74, 134, 68, 42);
    bypassButton.setBounds (832, 134, 156, 42);

    auto layoutKnobs = [] (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs, int nameHeight, int sliderHeight)
    {
        const int columnWidth = area.getWidth() / (int) knobs.size();
        for (auto* knob : knobs)
        {
            auto column = area.removeFromLeft (columnWidth);
            knob->nameArea = column.removeFromTop (nameHeight);
            knob->slider.setBounds (column.removeFromTop (sliderHeight).withSizeKeepingCentre (juce::jmin (column.getWidth(), sliderHeight - 10), sliderHeight));
        }
    };

    sampler.area = { 24, 212, 276, 256 };
    punch.area = { 312, 212, 276, 256 };
    tape.area = { 600, 212, 416, 256 };
    layoutKnobs (sampler.area.reduced (8, 0).withTrimmedTop (44), { &grit, &pitch }, 30, 172);
    layoutKnobs (punch.area.reduced (8, 0).withTrimmedTop (44), { &knock, &thump }, 30, 172);
    layoutKnobs (tape.area.reduced (8, 0).withTrimmedTop (44), { &warmth, &dust, &wobble }, 30, 172);
    pitch33Button.setBounds (sampler.area.getRight() - 104, sampler.area.getY() + 11, 78, 30);

    toneStereo.area = { 24, 482, 404, 238 };
    layoutKnobs (juce::Rectangle<int> (34, 526, 260, 190), { &lowCut, &width }, 28, 150);
    monoBassButton.setBounds (296, 566, 120, 40);
    sidechainButton.setBounds (296, 616, 120, 40);

    outputSection.area = { 440, 482, 344, 238 };
    layoutKnobs (juce::Rectangle<int> (448, 526, 220, 190), { &mix, &output }, 28, 150);
    outMeterArea = { 700, 540, 22, 140 };
    grMeterArea = { 752, 540, 22, 140 };

    sourceSection.area = { 796, 482, 220, 238 };
    sourceInput.setBounds (814, 530, 184, 38);
    sourceDemo.setBounds (814, 576, 184, 38);
    sourceFile.setBounds (814, 622, 184, 38);
    fileLabel.setBounds (808, 668, 196, 22);

    if (! background.isValid())
        renderBackground();
}

//==============================================================================
BirchBoxEditor::BirchBoxEditor (BirchBoxProcessor& p)
    : AudioProcessorEditor (p), view (p, lnf)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (view);

    constexpr double w = BirchView::baseWidth, h = BirchView::baseHeight;
    setResizable (true, true);
    setResizeLimits ((int) (w * 0.6), (int) (h * 0.6), (int) (w * 2.0), (int) (h * 2.0));
    getConstrainer()->setFixedAspectRatio (w / h);
    setSize ((int) w, (int) h);
}

BirchBoxEditor::~BirchBoxEditor()
{
    setLookAndFeel (nullptr);
}

void BirchBoxEditor::resized()
{
    view.setBounds (0, 0, BirchView::baseWidth, BirchView::baseHeight);
    view.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) BirchView::baseWidth));
}
