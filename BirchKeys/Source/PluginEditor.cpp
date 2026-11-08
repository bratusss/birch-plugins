#include "PluginEditor.h"
#include "BinaryData.h"
#include "Presets.h"

using namespace birchui;

namespace
{
    const bk::InstrumentDef& instrumentAt (int index)
    {
        const auto& list = bk::instruments();
        return list[(size_t) juce::jlimit (0, (int) list.size() - 1, index)];
    }

    int instrumentIndex (const juce::String& name)
    {
        const auto& list = bk::instruments();
        for (size_t i = 0; i < list.size(); ++i)
            if (name == list[i].name)
                return (int) i;
        return 0;
    }

    juce::String tabName (const juce::String& category)
    {
        return category == "Strings & Winds" ? juce::String ("STRINGS") : category.toUpperCase();
    }

    // what CHARACTER and MOTION mean for each kind of instrument
    juce::String characterName (const bk::InstrumentDef& d)
    {
        switch (d.engine)
        {
            case bk::Engine::modal:  return d.pickup != bk::Pickup::none ? "BARK" : "HARDNESS";
            case bk::Engine::string: return "PLUCK";
            case bk::Engine::organ:  return "PERC";
            case bk::Engine::analog: return "RESO";
            case bk::Engine::fm:     return "BRIGHT";
        }
        return "CHARACTER";
    }

    juce::String motionName (const bk::InstrumentDef& d)
    {
        switch (d.motion)
        {
            case bk::Motion::stereoTremolo:
            case bk::Motion::tremolo:    return "TREMOLO";
            case bk::Motion::vibesMotor: return "MOTOR";
            case bk::Motion::leslie:     return "LESLIE";
            case bk::Motion::vibrato:    return "VIBRATO";
            case bk::Motion::chorus:     return "CHORUS";
            case bk::Motion::none:       break;
        }
        return "MOTION";
    }

    juce::String characterTip (const bk::InstrumentDef& d)
    {
        switch (d.engine)
        {
            case bk::Engine::modal:
                return d.pickup != bk::Pickup::none
                    ? lv ("Cik tuvu pickup svārstās tine/mēlīte: vairāk = vairāk „bark” stipros sitienos.")
                    : lv ("Āmura/vāles cietība: vairāk = spožāks piesitiens un vairāk virstoņu.");
            case bk::Engine::string: return lv ("Pirksta/plektra cietība: vairāk = spožāks, „sitīgāks” piesitiens.");
            case bk::Engine::organ:  return lv ("Percussion skaļums: īsa harmonika katras frāzes pirmajai notij (kā Hammond).");
            case bk::Engine::analog: return lv ("Filtra rezonanse un piesātinājums.");
            case bk::Engine::fm:     return lv ("FM spožums: visu modulācijas indeksu reizinātājs.");
        }
        return {};
    }

    juce::String motionTip (const bk::InstrumentDef& d)
    {
        switch (d.motion)
        {
            case bk::Motion::stereoTremolo: return lv ("Suitcase stereo tremolo: skaņa „lēkā” starp skaļruņiem.");
            case bk::Motion::tremolo:       return lv ("Tremolo: skaļuma pulsēšana.");
            case bk::Motion::vibesMotor:    return lv ("Vibrafona motors: rezonatoru vārsti rada tremolo.");
            case bk::Motion::leslie:        return lv ("Leslie: līdz 55 % lēni (chorale), virs, ātri (tremolo). Mod wheel arī ieslēdz ātro.");
            case bk::Motion::vibrato:       return lv ("Vibrato, kas iestājas pēc mirkļa, kā dzīvam mūziķim.");
            case bk::Motion::chorus:        return lv ("BBD stereo chorus kā Juno: divas aizkaves, kas šūpojas pretfāzē.");
            case bk::Motion::none:          break;
        }
        return lv ("Šim instrumentam nav kustības efekta.");
    }

    juce::String engineName (const bk::InstrumentDef& d)
    {
        switch (d.engine)
        {
            case bk::Engine::modal:
                if (d.pianoPartials) return "STRINGS & HAMMER";
                if (d.pickup == bk::Pickup::magnetic) return "TINE & PICKUP";
                if (d.pickup == bk::Pickup::electrostatic) return "REED & PICKUP";
                return "BARS & MALLET";
            case bk::Engine::string: return "STRING & BODY";
            case bk::Engine::organ:  return "DRAWBARS";
            case bk::Engine::analog: return "SYNTH";
            case bk::Engine::fm:     return "FM OPERATORS";
        }
        return {};
    }
}

//==============================================================================
BirchKeyboard::BirchKeyboard (BirchKeysProcessor& p)
    : juce::MidiKeyboardComponent (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setAvailableRange (24, 96);
    setLowestVisibleKey (24);
    setScrollButtonsVisible (false);
    setKeyPressBaseOctave (5);
    setVelocity (0.8f, true);
    setOctaveForMiddleC (5);
    setWantsKeyboardFocus (true);
}

void BirchKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver,
                                   juce::Colour, juce::Colour)
{
    auto key = area.reduced (1.0f, 0.0f);
    juce::Colour top (0xfff5ead0), bottom (0xffd8c59c);
    if (isOver && ! isDown)
        top = top.brighter (0.08f);
    g.setGradientFill (juce::ColourGradient (top, 0.0f, key.getY(), bottom, 0.0f, key.getBottom(), false));
    g.fillRoundedRectangle (key.withTrimmedTop (-4.0f), 4.0f);
    if (isDown)
    {
        juce::Path shape;
        shape.addRoundedRectangle (key, 4.0f);
        fillGold (g, shape, 0.35f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawRoundedRectangle (key.withTrimmedTop (-4.0f), 4.0f, 1.2f);

    if (note % 12 == 0)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.setFont (BirchLookAndFeel::uiFont (10.0f));
        g.drawText (BirchKeysProcessor::noteName (note), key.removeFromBottom (15.0f), juce::Justification::centred);
    }
}

void BirchKeyboard::drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour)
{
    auto key = area.reduced (0.5f, 0.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (key.expanded (1.0f, 0.0f).withTrimmedTop (-3.0f), 3.0f);
    juce::Colour top (0xff2b2520), bottom (0xff080706);
    if (isDown)
    {
        top = colours::gold;
        bottom = colours::goldDark;
    }
    else if (isOver)
    {
        top = top.brighter (0.2f);
    }
    auto face = key.reduced (2.5f, 0.0f).withTrimmedBottom (5.0f);
    g.setGradientFill (juce::ColourGradient (top, 0.0f, face.getY(), bottom, 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (face, 2.5f);
    g.setColour (juce::Colours::white.withAlpha (isDown ? 0.3f : 0.08f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.25f), 2.0f);
}

//==============================================================================
BirchKeysView::BirchKeysView (BirchKeysProcessor& p, BirchLookAndFeel& l)
    : proc (p), lnf (l), keyboard (p)
{
    logo = juce::ImageCache::getFromMemory (BinaryData::birch_logo_png, BinaryData::birch_logo_pngSize);

    // ---- header: categories and presets
    for (const auto& d : bk::instruments())
        categories.addIfNotAlreadyThere (d.category);
    for (const auto& category : categories)
    {
        auto tab = std::make_unique<juce::TextButton> (tabName (category));
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (3001, juce::dontSendNotification);
        tab->getProperties().set ("graffiti", true);
        tab->setTooltip (category == "Bass" ? lv ("Basi: atsevišķi instrumenti, mono kā īstam basistam.")
                                            : lv ("Instrumentu kategorija."));
        tab->onClick = [this, category] { selectCategory (category); };
        addAndMakeVisible (*tab);
        categoryTabs.push_back (std::move (tab));
    }

    presetBox.setTooltip (lv ("Preseti šai kategorijai. Savējie ir sadaļā USER."));
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 10000)
        {
            const int index = id - 10000;
            if (juce::isPositiveAndBelow (index, userPresetFiles.size()))
                proc.loadUserPreset (userPresetFiles[index]);
        }
        else if (id > 0)
        {
            proc.setCurrentProgram (id - 1);
        }
    };
    addAndMakeVisible (presetBox);

    presetPrev.getProperties().set ("arrow", -1);
    presetNext.getProperties().set ("arrow", 1);
    auto stepPreset = [this] (int delta)
    {
        // walk through the factory presets of the current category
        const auto& presets = factoryPresets();
        std::vector<int> inCategory;
        for (size_t i = 0; i < presets.size(); ++i)
            if (currentCategory == instrumentAt (instrumentIndex (presets[i].instrument)).category)
                inCategory.push_back ((int) i);
        if (inCategory.empty())
            return;
        const auto it = std::find (inCategory.begin(), inCategory.end(), proc.getCurrentFactoryPreset());
        int pos = it == inCategory.end() ? 0 : (int) (it - inCategory.begin()) + delta;
        pos = (pos + (int) inCategory.size()) % (int) inCategory.size();
        proc.setCurrentProgram (inCategory[(size_t) pos]);
    };
    presetPrev.onClick = [stepPreset] { stepPreset (-1); };
    presetNext.onClick = [stepPreset] { stepPreset (1); };
    presetPrev.setTooltip (lv ("Iepriekšējais preset."));
    presetNext.setTooltip (lv ("Nākamais preset."));
    addAndMakeVisible (presetPrev);
    addAndMakeVisible (presetNext);

    saveButton.setButtonText ("SAVE");
    saveButton.setTooltip (lv ("Saglabā pašreizējo skaņu kā savu presetu."));
    saveButton.onClick = [this] { savePreset(); };
    addAndMakeVisible (saveButton);

    // ---- sound
    instrumentBox.setTooltip (lv ("Instruments. Visi ir fizikāli modelēti (bez sempliem), skat. RESEARCH.md."));
    instrumentBox.onChange = [this]
    {
        const int id = instrumentBox.getSelectedId();
        if (id > 0 && id - 1 != (int) proc.apvts.getRawParameterValue ("inst")->load())
            proc.selectInstrument (id - 1);
    };
    addAndMakeVisible (instrumentBox);

    setupKnob (level, "level", "LEVEL", lv ("Skaļums."), true);
    setupKnob (tone, "tone", "TONE", lv ("Tumšāks / spožāks. 50 % = instruments kā paredzēts."), true);
    setupKnob (decay, "decay", "DECAY", lv ("Cik ilgi skan turēta nots. 50 % = dabiski."), true);
    setupKnob (release, "release", "RELEASE", lv ("Cik ātri apklust pēc taustiņa atlaišanas (damper)."), true);
    setupKnob (character, "char", "CHARACTER", {}, true);
    setupKnob (motion, "motion", "MOTION", {}, true);
    setupKnob (rate, "rate", "RATE", lv ("Tremolo / Leslie / vibrato ātrums."), true);
    setupKnob (velocity, "velsens", "VELOCITY", lv ("Jutība pret sitiena stiprumu. 0 = visas notis vienādi stipras."), true);
    setupKnob (width, "width", "WIDTH", lv ("Stereo platums pa klaviatūru (zemās pa kreisi, augstās pa labi)."), true);
    setupKnob (attack, "attack", "ATTACK", lv ("Lēns ieskaņojums: no sitiena līdz „uzbriestošai” skaņai."), true);
    setupKnob (glide, "glide", "GLIDE", lv ("Portamento starp notīm (mono režīmā)."), true);
    setupKnob (tune, "tune", "TUNE", lv ("Precīza noskaņa centos: noderīga, lai pieskaņotos semplam."), true);

    monoButton.setButtonText ("MONO");
    monoButton.setClickingTogglesState (true);
    monoButton.getProperties().set ("led", true);
    monoButton.setTooltip (lv ("Viena nots vienlaicīgi (kā basists). Ar Glide notis slīd."));
    addAndMakeVisible (monoButton);
    monoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "mono", monoButton);
    setupStepper (octave, "octave", "OCT ", lv ("Oktāva."));
    setupStepper (bend, "bend", "BEND ", lv ("Pitch bend diapazons pustoņos."));

    // ---- engine pages
    setupKnob (bell, "bell", "BELL", {}, false);
    setupKnob (noise, "noise", "THUMP", {}, false);
    setupKnob (detune, "detune", "DETUNE", lv ("Cik nesaskaņotas ir unisona stīgas: vairāk = „honky”, pukstoši pianīno."), false);
    setupKnob (stretch, "stretch", "STRETCH", lv ("Stīgu neharmoniskums (stingums): augstākie virstoņi kļūst augstāki."), false);
    setupKnob (pickupOffset, "pkoffset", "PICKUP", lv ("Tine/mēlītes vieta pret pickup: 0 % = centrā (oktāva, zvans), 100 % = malā (pamattonis, growl). Ap 55 % 2. harmonika izzūd."), false);

    setupKnob (pluck, "pluck", "POSITION", lv ("Kur stīga tiek raustīta: tuvu galam = spožs un plāns, uz vidu = apaļš."), false);
    setupKnob (body, "body", "BODY", lv ("Korpusa rezonanses daudzums (koka skaņa)."), false);
    setupKnob (finger, "finger", "FINGER", lv ("Pirksta troksnis piesitienā."), false);
    setupKnob (pickup, "pickup", "PICKUP", lv ("Pickup vieta: maina vidējo frekvenču raksturu (comb filtrs). 0 = akustisks."), false);
    setupKnob (drop, "drop", "PITCH DROP", lv ("Stīga sākumā skan nedaudz augstāk un nostabilizējas (spriegums)."), false);
    setupKnob (mwah, "mwah", "MWAH", lv ("Fretless formants, kas pēc piesitiena slīd no ~1,4 kHz uz ~450 Hz."), false);

    const char* footages[] = { "16'", "5⅓'", "8'", "4'", "2⅔'", "2'", "1⅗'", "1⅓'", "1'" };
    const bool octaveBar[] = { true, false, true, true, false, true, false, false, true };
    for (int i = 0; i < 9; ++i)
    {
        auto& s = drawbars[i];
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.getProperties().set ("drawbar", true);
        s.getProperties().set ("octave", octaveBar[i]);
        s.setTooltip (lv ("Drawbar ") + juce::String::fromUTF8 (footages[i]) + lv (": šīs harmonikas skaļums (0–8)."));
        addAndMakeVisible (s);
        drawbarAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "db" + juce::String (i + 1), s);
    }
    setupRadio (percHarmonic, { "2ND", "3RD" }, "perch", 3101, lv ("Percussion harmonika: 2. (4′) vai 3. (2⅔′)."), false);
    setupKnob (percDecay, "percdecay", "PERC DECAY", lv ("Cik ātri izdziest percussion."), false);
    setupKnob (click, "click", "CLICK", lv ("Taustiņa kontaktu klikšķis."), false);
    setupKnob (chiff, "chiff", "CHIFF", lv ("Stabules „runas” troksnis ieskaņojoties (30 ms ap 2.–3. virstoni)."), false);
    for (int i = 0; i < 3; ++i)
    {
        const juce::String n (i + 1);
        setupKnob (fmRatio[i], "fm" + n + "r", "PAIR " + n + " RATIO", lv ("Modulatora frekvence pret nesēju. Veseli skaitļi = harmonisks, 14 = stikla „ping”."), false);
        setupKnob (fmIndex[i], "fm" + n + "i", "INDEX", lv ("Modulācijas dziļums: vairāk = vairāk harmoniku."), false);
        setupKnob (fmDecay[i], "fm" + n + "d", "IDX DECAY", lv ("Cik ātri indekss krīt: ātri = „knikšķis” tikai piesitienā."), false);
    }

    setupRadio (osc1Wave, { "SIN", "TRI", "SAW", "SQR" }, "osc1", 3102, lv ("Oscilatora 1 viļņa forma."), false);
    setupRadio (osc2Wave, { "SIN", "TRI", "SAW", "SQR" }, "osc2", 3103, lv ("Oscilatora 2 viļņa forma."), false);
    setupKnob (osc2Level, "osc2lvl", "OSC 2", lv ("Oscilatora 2 skaļums."), false);
    setupKnob (osc2Semis, "osc2semi", "SEMI", lv ("Oscilatora 2 nobīde pustoņos."), false);
    setupKnob (osc2Cents, "osc2cents", "DETUNE", lv ("Oscilatora 2 detune: biezāka skaņa."), false);
    setupKnob (sub, "sub", "SUB", lv ("Sub oscilators oktāvu zemāk."), false);
    setupKnob (noiseLevel, "noiselvl", "NOISE", lv ("Baltais troksnis."), false);
    setupKnob (breath, "breath", "BREATH", lv ("Elpas troksnis, filtrēts ap toni (flautai)."), false);
    setupKnob (cutoff, "cutoff", "CUTOFF", lv ("Ladder filtra frekvence."), false);
    setupKnob (envAmount, "envamt", "ENV", lv ("Cik daudz filtrs atveras piesitienā (oktāvās)."), false);
    setupKnob (filterDecay, "fdecay", "F.DECAY", lv ("Cik ātri filtrs aizveras atpakaļ."), false);
    setupKnob (sustain, "sustain", "SUSTAIN", lv ("Skaļums, kamēr taustiņš turēts."), false);
    setupKnob (drive, "drive", "DRIVE", lv ("Piesātinājums pirms filtra."), false);
    setupKnob (unisonSpread, "unidetune", "SPREAD", lv ("Unison balsu izkliede (centos)."), false);
    setupStepper (unison, "unison", "VOICES ", lv ("Unison balsu skaits (1–3)."));

    engineInfo.setJustificationType (juce::Justification::topLeft);
    engineInfo.setColour (juce::Label::textColourId, colours::dimText);
    engineInfo.setFont (BirchLookAndFeel::uiFont (13.0f, false));
    engineInfo.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (engineInfo);

    // ---- sampler chain
    setupKnob (grit, "grit", "GRIT", lv ("Semplera raksturs: mazāk bitu un zemāka sample rate."), false);
    setupKnob (filter, "filter", "SP FILTER", lv ("4-polu low-pass kā SP-1200: griez uz leju, lai „nofiltrētu”."), false);
    setupKnob (reso, "reso", "RESO", lv ("SP filtra rezonanse."), false);
    setupKnob (warmth, "warmth", "WARMTH", lv ("Lentes saturācija."), false);
    setupKnob (wobble, "wobble", "WOBBLE", lv ("Plates/lentes wow & flutter."), false);
    setupKnob (dust, "dust", "DUST", lv ("Vinila troksnis un sprakšķi."), false);
    setupKnob (room, "room", "ROOM", lv ("Neliela telpa."), false);
    setupKnob (volume, "volume", "VOLUME", lv ("Kopējais skaļums."), false);
    setupRadio (era, { "'88", "'94", "'98" }, "era", 3104, lv ("'88 netīrs, '94 SP-1200 (26 kHz, 12 bit), '98 tīrāks (40 kHz)."), true);

    keyboard.setTooltip (lv ("Spēlē ar peli vai datora klaviatūru (A W S E D F T G Y H U J K)."));
    addAndMakeVisible (keyboard);

    lnf.setColour (juce::AlertWindow::backgroundColourId, colours::surface);
    lnf.setColour (juce::AlertWindow::textColourId, colours::text);
    lnf.setColour (juce::AlertWindow::outlineColourId, colours::goldDark);

    setSize (baseWidth, baseHeight);
    syncInstrument();
    startTimerHz (30);
}

BirchKeysView::~BirchKeysView()
{
    stopTimer();
}

void BirchKeysView::setupKnob (Knob& knob, const juce::String& paramId, const juce::String& name, const juce::String& tooltip, bool big)
{
    knob.name = name;
    knob.big = big;
    knob.slider.getProperties().set ("big", big);
    knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 92, 20);
    if (tooltip.isNotEmpty())
        knob.slider.setTooltip (tooltip);
    auto* param = proc.apvts.getParameter (paramId);
    knob.slider.setDoubleClickReturnValue (true, (double) param->convertFrom0to1 (param->getDefaultValue()));
    addAndMakeVisible (knob.slider);
    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, knob.slider);
}

void BirchKeysView::setupStepper (Stepper& s, const juce::String& paramId, const juce::String& prefix, const juce::String& tooltip)
{
    s.paramId = paramId;
    s.prefix = prefix;
    s.down.getProperties().set ("arrow", -1);
    s.up.getProperties().set ("arrow", 1);
    s.down.onClick = [this, paramId] { stepParam (paramId, -1); };
    s.up.onClick = [this, paramId] { stepParam (paramId, 1); };
    s.down.setTooltip (tooltip);
    s.up.setTooltip (tooltip);
    s.label.setJustificationType (juce::Justification::centred);
    s.label.setFont (BirchLookAndFeel::uiFont (12.0f));
    s.label.setColour (juce::Label::textColourId, colours::goldLight);
    s.label.setColour (juce::Label::backgroundColourId, colours::lcd);
    s.label.setColour (juce::Label::outlineColourId, colours::goldDark.withAlpha (0.6f));
    addAndMakeVisible (s.down);
    addAndMakeVisible (s.up);
    addAndMakeVisible (s.label);
}

void BirchKeysView::setupRadio (Radio& radio, const juce::StringArray& names, const juce::String& paramId, int group,
                                const juce::String& tooltip, bool graffiti)
{
    for (int i = 0; i < names.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (names[i]);
        b->setClickingTogglesState (true);
        b->setRadioGroupId (group, juce::dontSendNotification);
        b->setTooltip (tooltip);
        b->getProperties().set ("graffiti", graffiti);
        b->onClick = [&radio, i] { radio.attachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*b);
        radio.buttons.push_back (std::move (b));
    }
    radio.attachment = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter (paramId), [&radio] (float v)
    {
        for (size_t i = 0; i < radio.buttons.size(); ++i)
            radio.buttons[i]->setToggleState ((int) i == juce::roundToInt (v), juce::dontSendNotification);
    });
    radio.attachment->sendInitialUpdate();
}

void BirchKeysView::stepParam (const juce::String& id, int delta)
{
    auto* param = proc.apvts.getParameter (id);
    const auto& range = param->getNormalisableRange();
    const float v = juce::jlimit (range.start, range.end, param->convertFrom0to1 (param->getValue()) + (float) delta);
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 (v));
    param->endChangeGesture();
}

//==============================================================================
void BirchKeysView::selectCategory (const juce::String& category)
{
    // jump to the first preset of the category
    const auto& presets = factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        if (category == instrumentAt (instrumentIndex (presets[i].instrument)).category)
        {
            proc.setCurrentProgram ((int) i);
            break;
        }
    syncInstrument();
}

void BirchKeysView::rebuildInstrumentList()
{
    instrumentBox.clear (juce::dontSendNotification);
    instrumentIds.clear();
    const auto& list = bk::instruments();
    for (size_t i = 0; i < list.size(); ++i)
        if (currentCategory == list[i].category)
        {
            instrumentBox.addItem (list[i].name, (int) i + 1);
            instrumentIds.push_back ((int) i);
        }
}

void BirchKeysView::rebuildPresetList()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& presets = factoryPresets();
    juce::String heading;
    for (size_t i = 0; i < presets.size(); ++i)
    {
        const auto& def = instrumentAt (instrumentIndex (presets[i].instrument));
        if (currentCategory != def.category)
            continue;
        if (heading != presets[i].instrument)
        {
            heading = presets[i].instrument;
            presetBox.addSectionHeading (heading);
        }
        presetBox.addItem (presets[i].name, (int) i + 1);
    }

    userPresetFiles = proc.getUserPresets();
    if (! userPresetFiles.isEmpty())
    {
        presetBox.addSectionHeading ("USER");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem (userPresetFiles[i].getFileNameWithoutExtension(), 10000 + i);
    }
    presetCount = (int) presets.size() + userPresetFiles.size();
    shownPreset = -2; // force re-selection
}

void BirchKeysView::savePreset()
{
    auto* window = new juce::AlertWindow (lv ("Saglabāt presetu"), lv ("Nosaukums:"), juce::MessageBoxIconType::NoIcon, this);
    window->addTextEditor ("name", proc.getPresetName() + " 2");
    window->addButton (lv ("Saglabāt"), 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton (lv ("Atcelt"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
    window->enterModalState (true, juce::ModalCallbackFunction::create ([this, window] (int result)
    {
        if (result == 1 && proc.saveUserPreset (window->getTextEditorContents ("name")))
            rebuildPresetList();
    }), true);
}

void BirchKeysView::showPage (Page p)
{
    page = p;
    for (auto pg : { Page::modal, Page::string, Page::organ, Page::analog, Page::fm })
        for (auto* knob : pageKnobs (pg))
            knob->slider.setVisible (pg == p);
    for (auto& d : drawbars)
        d.setVisible (p == Page::organ);
    for (auto& b : percHarmonic.buttons)
        b->setVisible (p == Page::organ);
    for (auto* radio : { &osc1Wave, &osc2Wave })
        for (auto& b : radio->buttons)
            b->setVisible (p == Page::analog);
    for (auto* c : { (juce::Component*) &unison.down, (juce::Component*) &unison.up, (juce::Component*) &unison.label })
        c->setVisible (p == Page::analog);
    engineInfo.setVisible (p == Page::modal || p == Page::string);
}

void BirchKeysView::syncInstrument()
{
    const int index = (int) proc.apvts.getRawParameterValue ("inst")->load();
    const auto& def = instrumentAt (index);

    if (currentCategory != def.category)
    {
        currentCategory = def.category;
        for (size_t i = 0; i < categoryTabs.size(); ++i)
            categoryTabs[i]->setToggleState (categories[(int) i] == currentCategory, juce::dontSendNotification);
        rebuildInstrumentList();
        rebuildPresetList();
    }
    instrumentBox.setSelectedId (index + 1, juce::dontSendNotification);

    if (index == shownInstrument)
        return;
    shownInstrument = index;

    character.name = characterName (def);
    motion.name = motionName (def);
    character.slider.setTooltip (characterTip (def));
    motion.slider.setTooltip (motionTip (def));
    const bool hasMotion = def.motion != bk::Motion::none;
    const bool hasRate = hasMotion && def.motion != bk::Motion::leslie; // Leslie speeds come from MOTION
    motion.slider.setEnabled (hasMotion);
    motion.slider.setAlpha (hasMotion ? 1.0f : 0.35f);
    rate.slider.setEnabled (hasRate);
    rate.slider.setAlpha (hasRate ? 1.0f : 0.35f);

    switch (def.engine)
    {
        case bk::Engine::modal:  showPage (Page::modal); break;
        case bk::Engine::string: showPage (Page::string); break;
        case bk::Engine::organ:  showPage (Page::organ); break;
        case bk::Engine::analog: showPage (Page::analog); break;
        case bk::Engine::fm:     showPage (Page::fm); break;
    }

    if (def.engine == bk::Engine::modal)
    {
        // the same two knobs mean different physical things per instrument
        bell.name = def.pianoPartials ? "HAMMER" : "BELL";
        bell.slider.setTooltip (def.pianoPartials ? lv ("Āmura spožums: cik daudz augsto virstoņu ierosina sitiens.")
                                                  : lv ("Augstāko modu (zvaniņa, virstoņu) skaļums."));
        noise.name = def.pickup != bk::Pickup::none || def.pianoPartials ? "THUMP" : "CLICK";
        noise.slider.setTooltip (lv ("Āmura/vāles piesitiena troksnis."));
        detune.slider.setVisible (def.pianoPartials);
        stretch.slider.setVisible (def.pianoPartials);
        pickupOffset.slider.setVisible (def.pickup == bk::Pickup::magnetic);
    }
    engineTitle = engineName (def);
    engineInfo.setText (lv (def.physics), juce::dontSendNotification);
    resized();
    repaint();
}

void BirchKeysView::timerCallback()
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
    repaint (meterArea.expanded (8, 24));
    repaint (logoArea.expanded (40));

    syncInstrument();

    // keep the preset box showing the current preset
    const int factory = proc.getCurrentFactoryPreset();
    if (factory != shownPreset || proc.getPresetName() != shownPresetName)
    {
        shownPreset = factory;
        shownPresetName = proc.getPresetName();
        if (factory >= 0)
            presetBox.setSelectedId (factory + 1, juce::dontSendNotification);
        else
        {
            int id = 0;
            for (int i = 0; i < userPresetFiles.size(); ++i)
                if (userPresetFiles[i].getFileNameWithoutExtension() == juce::File::createLegalFileName (shownPresetName))
                    id = 10000 + i;
            if (id > 0)
                presetBox.setSelectedId (id, juce::dontSendNotification);
            else
                presetBox.setText (shownPresetName, juce::dontSendNotification);
        }
    }

    for (auto* s : { &octave, &bend, &unison })
    {
        const int v = (int) proc.apvts.getRawParameterValue (s->paramId)->load();
        s->label.setText (s->prefix + (s == &octave && v > 0 ? "+" : "") + juce::String (v), juce::dontSendNotification);
    }

    // let the computer keyboard play notes in the standalone app
    if (! focusGrabbed && keyboard.isShowing() && juce::JUCEApplicationBase::isStandaloneApp())
    {
        keyboard.grabKeyboardFocus();
        focusGrabbed = true;
    }

    // Dev aid: BIRCHKEYS_SNAPSHOT=/path/file.png saves a picture of the UI once it has run a moment.
    // BIRCHKEYS_INSTRUMENT=<index> picks the instrument first, to check every engine page.
    static int ticks = 0;
    ++ticks;
    const auto forced = juce::SystemStats::getEnvironmentVariable ("BIRCHKEYS_INSTRUMENT", {});
    if (forced.isNotEmpty() && ticks == 5)
        proc.selectInstrument (forced.getIntValue());
    const auto snapshotPath = juce::SystemStats::getEnvironmentVariable ("BIRCHKEYS_SNAPSHOT", {});
    if (snapshotPath.isNotEmpty() && ticks == 45)
    {
        auto* target = getParentComponent() != nullptr ? getParentComponent() : this;
        juce::File file (snapshotPath);
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat().writeImageToStream (target->createComponentSnapshot (target->getLocalBounds(), true, 1.0f), stream);
    }
}

//==============================================================================
std::vector<BirchKeysView::Knob*> BirchKeysView::soundKnobs()
{
    return { &level, &tone, &decay, &release, &character, &motion, &rate, &velocity, &width, &attack, &glide, &tune };
}

std::vector<BirchKeysView::Knob*> BirchKeysView::pageKnobs (Page p)
{
    switch (p)
    {
        case Page::modal:  return { &bell, &noise, &detune, &stretch, &pickupOffset };
        case Page::string: return { &pluck, &body, &finger, &pickup, &drop, &mwah };
        case Page::organ:  return { &percDecay, &click, &chiff };
        case Page::fm:     return { &fmRatio[0], &fmIndex[0], &fmDecay[0], &fmRatio[1], &fmIndex[1], &fmDecay[1], &fmRatio[2], &fmIndex[2], &fmDecay[2] };
        case Page::analog: return { &osc2Level, &osc2Semis, &osc2Cents, &sub, &noiseLevel, &breath,
                                    &cutoff, &envAmount, &filterDecay, &sustain, &drive, &unisonSpread };
    }
    return {};
}

void BirchKeysView::paintSection (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& title, bool baked)
{
    const auto r = area.toFloat();
    if (baked)
    {
        juce::Path shape;
        shape.addRoundedRectangle (r, 14.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.85f), 26, { 0, 10 }).drawForPath (g, shape);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xf41c1711), 0.0f, r.getY(), juce::Colour (0xf80a0806), 0.0f, r.getBottom(), false));
        g.fillPath (shape);
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
            drawScrew (g, corner + juce::Point<float> (corner.x < r.getCentreX() ? 13.0f : -13.0f, corner.y < r.getCentreY() ? 13.0f : -13.0f), angle);
    }

    if (title.isNotEmpty())
    {
        const auto font = lnf.graffitiFont (26.0f);
        const auto box = juce::Rectangle<float> (r.getX() + 30.0f, r.getY() + 8.0f, r.getWidth() - 60.0f, 32.0f);
        drawGraffiti (g, font, title, box, juce::Justification::centredLeft, 4.0f);
        if (baked)
        {
            const float lineX = box.getX() + textPath (font, title, box, juce::Justification::centredLeft).getBounds().getWidth() + 12.0f;
            const float lineY = box.getCentreY() + 1.0f;
            const float lineEnd = box.getRight();
            g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.55f), lineX, lineY, colours::gold.withAlpha (0.0f), lineEnd, lineY, false));
            g.fillRect (juce::Rectangle<float> (lineX, lineY - 1.0f, juce::jmax (0.0f, lineEnd - lineX), 2.0f));
        }
    }
}

void BirchKeysView::renderBackground()
{
    constexpr int scale = 2;
    background = juce::Image (juce::Image::RGB, baseWidth * scale, baseHeight * scale, true);
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
                const juce::Rectangle<float> brick (x + mortar * 0.5f, (float) row * brickH + mortar * 0.5f, brickW - mortar, brickH - mortar);
                const float shade = 0.09f + rnd.nextFloat() * 0.045f;
                const auto c = juce::Colour::fromFloatRGBA (shade * 1.18f, shade * 0.98f, shade * 0.8f, 1.0f);
                g.setGradientFill (juce::ColourGradient (c.brighter (0.25f), 0.0f, brick.getY(), c.darker (0.45f), 0.0f, brick.getBottom(), false));
                g.fillRoundedRectangle (brick, 2.5f);
            }
        }
    }
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
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.25f), baseWidth * 0.5f, baseHeight * 0.4f,
                                             juce::Colours::black.withAlpha (0.92f), 0.0f, 0.0f, true));
    g.fillAll();
    const auto logoCentre = logoArea.getCentre().toFloat();
    g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.14f), logoCentre.x, logoCentre.y,
                                             juce::Colours::transparentBlack, logoCentre.x + 330.0f, logoCentre.y, true));
    g.fillRect (getLocalBounds());

    {
        auto area = titleArea.toFloat();
        constexpr float barHeight = 12.0f;
        auto top = area.removeFromTop ((area.getHeight() - barHeight) * 0.5f);
        auto bar = area.removeFromTop (barHeight);
        auto bottom = area;
        drawGraffiti (g, lnf.graffitiFont (top.getHeight() * 1.22f), "BIRCH", top.expanded (0, 8), juce::Justification::centred, 8.0f, true);
        drawGoldBar (g, bar.reduced (0, 1));
        drawGraffiti (g, lnf.graffitiFont (bottom.getHeight() * 1.22f), "KEYS", bottom.expanded (0, 8), juce::Justification::centred, 8.0f, true);
        g.setColour (colours::dimText);
        g.setFont (BirchLookAndFeel::uiFont (12.0f).withExtraKerningFactor (0.45f));
        g.drawText ("BOOM BAP INSTRUMENTS", titleArea.withY (titleArea.getBottom() + 6).withHeight (16), juce::Justification::centred);
    }

    paintSection (g, headerArea, {});
    paintSection (g, soundArea, "SOUND");
    paintSection (g, engineArea, {});
    paintSection (g, fxArea, "SAMPLER & VINYL");
    paintSection (g, keyboardArea, {});

    g.setColour (colours::dimText);
    g.setFont (BirchLookAndFeel::uiFont (11.0f).withExtraKerningFactor (0.3f));
    g.drawText ("PRESET", presetBox.getBounds().translated (2, -19).withHeight (15), juce::Justification::centredLeft);
    g.drawText ("INSTRUMENT", instrumentBox.getBounds().translated (2, -18).withHeight (14), juce::Justification::centredLeft);
    g.drawText ("ERA", era.buttons[0]->getBounds().translated (2, -17).withHeight (14), juce::Justification::centredLeft);

    for (auto* knob : { &level, &tone, &decay, &release, &velocity, &width, &attack, &glide, &tune,
                        &grit, &filter, &reso, &warmth, &wobble, &dust, &room, &volume })
        drawGraffiti (g, lnf.graffitiFont (knob->big ? 22.0f : 20.0f), knob->name, knob->nameArea.toFloat(), juce::Justification::centred, 4.0f);

    drawGraffiti (g, lnf.graffitiFont (18.0f), "OUT", meterArea.toFloat().withY ((float) meterArea.getBottom() + 3.0f).withHeight (22.0f).expanded (20.0f, 0.0f),
                  juce::Justification::centred, 3.5f);

    logo = logo.rescaled (logoArea.getWidth() * scale, logoArea.getHeight() * scale, juce::Graphics::highResamplingQuality);
}

void BirchKeysView::paint (juce::Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    // logo breathes with the music
    const float lvl = juce::jlimit (0.0f, 1.0f, meterLevel * 1.3f);
    const auto c = logoArea.getCentre().toFloat();
    g.setGradientFill (juce::ColourGradient (colours::gold.withAlpha (0.05f + 0.28f * lvl), c.x, c.y,
                                             juce::Colours::transparentBlack, c.x + logoArea.getWidth() * 0.62f, c.y, true));
    g.fillEllipse (logoArea.toFloat().expanded (40.0f));
    g.drawImage (logo, logoArea.toFloat());

    // engine panel title and the knob names that change with the instrument
    paintSection (g, engineArea, engineTitle, false);
    std::vector<Knob*> dynamic { &character, &motion, &rate };
    for (auto* k : pageKnobs (page))
        dynamic.push_back (k);
    for (auto* knob : dynamic)
    {
        if (! knob->slider.isVisible())
            continue;
        juce::Graphics::ScopedSaveState state (g);
        if (! knob->slider.isEnabled())
            g.setOpacity (0.35f);
        drawGraffiti (g, lnf.graffitiFont (knob->big ? 22.0f : 19.0f), knob->name, knob->nameArea.toFloat(), juce::Justification::centred, 4.0f);
    }

    if (page == Page::organ)
    {
        const char* footages[] = { "16'", "5⅓'", "8'", "4'", "2⅔'", "2'", "1⅗'", "1⅓'", "1'" };
        g.setColour (colours::text);
        g.setFont (BirchLookAndFeel::uiFont (12.0f));
        for (int i = 0; i < 9; ++i)
            g.drawText (juce::String::fromUTF8 (footages[i]), drawbars[i].getBounds().withY (drawbars[i].getBottom() + 4).withHeight (16).expanded (6, 0),
                        juce::Justification::centred);
        g.setColour (colours::dimText);
        g.setFont (BirchLookAndFeel::uiFont (11.0f).withExtraKerningFactor (0.3f));
        g.drawText ("PERCUSSION", percHarmonic.buttons[0]->getBounds().translated (2, -17).withHeight (14).withWidth (120), juce::Justification::centredLeft);
    }
    if (page == Page::analog)
    {
        g.setColour (colours::dimText);
        g.setFont (BirchLookAndFeel::uiFont (11.0f).withExtraKerningFactor (0.3f));
        g.drawText ("OSC 1", osc1Wave.buttons[0]->getBounds().translated (2, -17).withHeight (14).withWidth (80), juce::Justification::centredLeft);
        g.drawText ("OSC 2", osc2Wave.buttons[0]->getBounds().translated (2, -17).withHeight (14).withWidth (80), juce::Justification::centredLeft);
    }

    // segmented output meter
    const auto r = meterArea.toFloat();
    g.setColour (colours::lcd);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (colours::goldDark.withAlpha (0.45f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    const float fill = (juce::Decibels::gainToDecibels (meterLevel, -60.0f) + 60.0f) / 60.0f;
    const float peakFill = (juce::Decibels::gainToDecibels (peakLevel, -60.0f) + 60.0f) / 60.0f;
    constexpr int segments = 20;
    const auto inner = r.reduced (4.0f);
    const float segH = (inner.getHeight() - 2.0f * (segments - 1)) / segments;
    for (int i = 0; i < segments; ++i)
    {
        const float t = ((float) i + 0.5f) / (float) segments;
        const juce::Rectangle<float> seg (inner.getX(), inner.getBottom() - (float) (i + 1) * segH - (float) i * 2.0f, inner.getWidth(), segH);
        const auto col = i >= segments - 3 ? colours::danger : i >= segments - 7 ? colours::goldLight : colours::gold;
        const bool lit = t <= fill || std::abs (t - peakFill) < 0.5f / segments;
        if (lit)
        {
            g.setColour (col.withAlpha (0.25f));
            g.fillRoundedRectangle (seg.expanded (2.0f, 1.5f), 2.0f);
        }
        g.setColour (lit ? col : col.withAlpha (0.07f));
        g.fillRoundedRectangle (seg, 1.5f);
    }
}

void BirchKeysView::resized()
{
    logoArea = { 26, 14, 196, 183 };
    titleArea = { 236, 34, 290, 136 };

    // header: category tabs over the preset browser
    headerArea = { 552, 20, 604, 180 };
    for (size_t i = 0; i < categoryTabs.size(); ++i)
        categoryTabs[i]->setBounds (572 + (int) i * 114, 40, 108, 42);
    presetBox.setBounds (572, 120, 330, 44);
    presetPrev.setBounds (908, 120, 42, 44);
    presetNext.setBounds (954, 120, 42, 44);
    saveButton.setBounds (1004, 120, 132, 44);

    auto knobGrid = [] (std::vector<Knob*> knobs, juce::Rectangle<int> row, int nameHeight, int sliderHeight, int maxDial)
    {
        const int column = row.getWidth() / (int) knobs.size();
        for (auto* knob : knobs)
        {
            auto col = row.removeFromLeft (column);
            knob->nameArea = col.removeFromTop (nameHeight);
            knob->slider.setBounds (col.removeFromTop (sliderHeight).withSizeKeepingCentre (juce::jmin (col.getWidth(), maxDial), sliderHeight));
        }
    };

    // sound
    soundArea = { 24, 214, 640, 380 };
    monoButton.setBounds (306, 224, 84, 30);
    octave.down.setBounds (400, 224, 28, 30);
    octave.label.setBounds (430, 224, 58, 30);
    octave.up.setBounds (490, 224, 28, 30);
    bend.down.setBounds (526, 224, 28, 30);
    bend.label.setBounds (556, 224, 54, 30);
    bend.up.setBounds (612, 224, 28, 30);
    instrumentBox.setBounds (48, 276, 592, 44);
    knobGrid ({ &level, &tone, &decay, &release, &character, &motion }, { 34, 330, 620, 128 }, 24, 104, 96);
    knobGrid ({ &rate, &velocity, &width, &attack, &glide, &tune }, { 34, 460, 620, 128 }, 24, 104, 96);

    // engine
    engineArea = { 676, 214, 480, 380 };
    const int ex = engineArea.getX(), ey = engineArea.getY();
    {
        // modal: piano gets four knobs, the rest two
        std::vector<Knob*> knobs { &bell, &noise };
        if (detune.slider.isVisible())
        {
            knobs.push_back (&detune);
            knobs.push_back (&stretch);
        }
        if (pickupOffset.slider.isVisible())
            knobs.push_back (&pickupOffset);
        knobGrid (knobs, { ex + 20, ey + 54, 440, 120 }, 24, 96, 92);
        engineInfo.setBounds (ex + 30, ey + 190, 420, 170);
    }
    knobGrid ({ &pluck, &body, &finger, &pickup, &drop, &mwah }, { ex + 10, ey + 54, 460, 120 }, 24, 96, 74);
    if (page == Page::string)
        engineInfo.setBounds (ex + 30, ey + 190, 420, 170);
    // one operator pair per row
    for (int i = 0; i < 3; ++i)
        knobGrid ({ &fmRatio[i], &fmIndex[i], &fmDecay[i] }, { ex + 20, ey + 50 + i * 108, 440, 104 }, 22, 80, 76);

    for (int i = 0; i < 9; ++i)
        drawbars[i].setBounds (ex + 30 + i * 47, ey + 62, 40, 168);
    for (size_t i = 0; i < percHarmonic.buttons.size(); ++i)
        percHarmonic.buttons[i]->setBounds (ex + 30 + (int) i * 72, ey + 300, 66, 36);
    knobGrid ({ &percDecay, &click, &chiff }, { ex + 170, ey + 262, 300, 110 }, 22, 88, 80);

    for (size_t i = 0; i < osc1Wave.buttons.size(); ++i)
    {
        osc1Wave.buttons[i]->setBounds (ex + 24 + (int) i * 52, ey + 70, 48, 28);
        osc2Wave.buttons[i]->setBounds (ex + 248 + (int) i * 52, ey + 70, 48, 28);
    }
    unison.down.setBounds (ex + 300, ey + 12, 28, 30);
    unison.label.setBounds (ex + 330, ey + 12, 90, 30);
    unison.up.setBounds (ex + 422, ey + 12, 28, 30);
    knobGrid ({ &osc2Level, &osc2Semis, &osc2Cents, &sub, &noiseLevel, &breath }, { ex + 12, ey + 108, 456, 130 }, 22, 100, 74);
    knobGrid ({ &cutoff, &envAmount, &filterDecay, &sustain, &drive, &unisonSpread }, { ex + 12, ey + 242, 456, 130 }, 22, 100, 74);

    // sampler chain
    fxArea = { 24, 606, 1132, 146 };
    for (size_t i = 0; i < era.buttons.size(); ++i)
        era.buttons[i]->setBounds (46 + (int) i * 66, 690, 60, 40);
    knobGrid ({ &grit, &filter, &reso, &warmth, &wobble, &dust, &room, &volume }, { 254, 628, 800, 120 }, 22, 96, 92);
    meterArea = { 1090, 638, 24, 86 };

    keyboardArea = { 24, 764, 1132, 90 };
    keyboard.setBounds (36, 772, 1108, 74);
    keyboard.setKeyWidth (1108.0f / 43.0f);

    if (! background.isValid())
        renderBackground();
}

//==============================================================================
BirchKeysEditor::BirchKeysEditor (BirchKeysProcessor& p)
    : AudioProcessorEditor (p), view (p, lnf)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (view);
    constexpr double w = BirchKeysView::baseWidth, h = BirchKeysView::baseHeight;
    setResizable (true, true);
    setResizeLimits ((int) (w * 0.5), (int) (h * 0.5), (int) (w * 2.0), (int) (h * 2.0));
    getConstrainer()->setFixedAspectRatio (w / h);
    setSize ((int) (w * 0.85), (int) (h * 0.85));
}

BirchKeysEditor::~BirchKeysEditor()
{
    setLookAndFeel (nullptr);
}

void BirchKeysEditor::resized()
{
    view.setBounds (0, 0, BirchKeysView::baseWidth, BirchKeysView::baseHeight);
    view.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) BirchKeysView::baseWidth));
}
