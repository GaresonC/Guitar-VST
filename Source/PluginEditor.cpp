#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
// Colour palette
static const juce::Colour kBg        { 0xff141414 };
static const juce::Colour kPanel     { 0xff222222 };
static const juce::Colour kAccent    { 0xffff6600 };
static const juce::Colour kText      { 0xffe0e0e0 };
static const juce::Colour kSubText   { 0xff888888 };
static const juce::Colour kGreen     { 0xff00dd55 };
static const juce::Colour kRed       { 0xffdd2222 };
static const juce::Colour kBtnActive { 0xffcc4400 };
static const juce::Colour kBtnIdle   { 0xff333333 };

static const juce::String kIRDir     = "C:/Users/Gary/Documents/Cab IR/Custom IRs/";

//==============================================================================
void GuitarAmpLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle, juce::Slider& slider)
{
    const float cx = x + width  * 0.5f;
    const float cy = y + height * 0.5f;
    const float r  = juce::jmin(width, height) * 0.5f - 4.0f;
    const float toAngle = startAngle + sliderPos * (endAngle - startAngle);
    const auto accent = slider.findColour(juce::Slider::rotarySliderFillColourId);

    // Background track (full range, dark grey)
    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, endAngle, true);
    g.setColour(juce::Colour(0xff2a2a2a));
    g.strokePath(bgArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Value arc (start to current position, accent colour)
    if (sliderPos > 0.0f)
    {
        juce::Path valArc;
        valArc.addCentredArc(cx, cy, r, r, 0.0f, startAngle, toAngle, true);
        g.setColour(accent);
        g.strokePath(valArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    // Knob body (flat dark circle with subtle rim)
    const float br = r - 6.0f;
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillEllipse(cx - br, cy - br, br * 2.0f, br * 2.0f);
    g.setColour(juce::Colour(0xff333333));
    g.drawEllipse(cx - br, cy - br, br * 2.0f, br * 2.0f, 1.0f);

    // Indicator line from centre toward edge at current angle
    const float lineAngle = toAngle - juce::MathConstants<float>::halfPi;
    const float innerR    = br * 0.35f;
    g.setColour(accent.brighter(0.5f));
    g.drawLine(cx + innerR * std::cos(lineAngle),
               cy + innerR * std::sin(lineAngle),
               cx + br     * std::cos(lineAngle),
               cy + br     * std::sin(lineAngle), 2.0f);
}

//==============================================================================
TunerDisplay::TunerDisplay(GuitarAmpAudioProcessor& p) : processor(p)
{
    startTimerHz(15);
}

TunerDisplay::~TunerDisplay()
{
    stopTimer();
}

void TunerDisplay::timerCallback()
{
    current = processor.tuner.getResult();
    repaint();
}

void TunerDisplay::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    g.setColour(kPanel);
    g.fillRoundedRectangle(b, 5.0f);

    g.setColour(kSubText.withAlpha(0.4f));
    g.drawRoundedRectangle(b.reduced(0.5f), 5.0f, 1.0f);

    if (!current.hasSignal)
    {
        g.setColour(kSubText);
        g.setFont(juce::Font(14.0f, juce::Font::italic));
        g.drawText("-- no signal --", b, juce::Justification::centred);
        return;
    }

    const int note = current.midiNote;
    if (note < 0) return;

    const int   noteIndex = note % 12;
    const int   octave    = note / 12 - 1;
    const float cents     = current.cents;
    const bool  inTune    = std::abs(cents) < 5.0f;
    const juce::Colour ind = inTune ? kGreen : kRed;

    // Note name
    juce::String noteName = juce::String(Tuner::noteNames[noteIndex]) + juce::String(octave);
    g.setColour(kText);
    g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawText(noteName, b.withWidth(80.0f).withX(10.0f), juce::Justification::centred);

    // Cents value
    juce::String centsStr = (cents >= 0.0f ? "+" : "") + juce::String((int)cents) + "c";
    g.setColour(ind);
    g.setFont(juce::Font(14.0f));
    g.drawText(centsStr, b.withLeft(b.getRight() - 55.0f).withRight(b.getRight() - 5.0f),
               juce::Justification::centred);

    // Cents bar
    const float bx  = 95.0f;
    const float bw  = b.getWidth() - 165.0f;
    const float bh  = 10.0f;
    const float by  = (b.getHeight() - bh) * 0.5f;
    const float mid = bx + bw * 0.5f;

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRoundedRectangle(bx, by, bw, bh, 3.0f);

    // Centre tick
    g.setColour(kText.withAlpha(0.25f));
    g.fillRect(mid - 1.0f, by - 4.0f, 2.0f, bh + 8.0f);

    // Indicator dot
    const float norm = juce::jlimit(-50.0f, 50.0f, cents) / 50.0f;
    const float ix   = mid + norm * bw * 0.45f;
    g.setColour(ind);
    g.fillEllipse(ix - 6.0f, by - 2.0f, 12.0f, bh + 4.0f);
}

//==============================================================================
GuitarAmpAudioProcessorEditor::GuitarAmpAudioProcessorEditor(GuitarAmpAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tunerDisplay(p)
{
    juce::LookAndFeel::setDefaultLookAndFeel(&ampLookAndFeel);
    setSize(1108, 650);

    // ---- Tuner display + mute button -----------------------------------------
    addAndMakeVisible(tunerDisplay);

    tunerToggle.setClickingTogglesState(true);
    tunerToggle.setColour(juce::TextButton::buttonColourId,   kBtnIdle);
    tunerToggle.setColour(juce::TextButton::buttonOnColourId, kRed);
    tunerToggle.setColour(juce::TextButton::textColourOffId,  kText);
    tunerToggle.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    addAndMakeVisible(tunerToggle);
    muteEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "muteEnabled", tunerToggle);

    // ---- Noise gate ----------------------------------------------------------
    gateEnableBtn.setClickingTogglesState(true);
    styleButton(gateEnableBtn, true);
    addAndMakeVisible(gateEnableBtn);
    gateEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "noiseGateEnabled", gateEnableBtn);

    setupLargeKnob(gateThreshSlider, gateThreshLabel, "THRESHOLD");
    gateThreshAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "noiseGateThreshold", gateThreshSlider);

    // ---- Pre-amp EQ ----------------------------------------------------------
    setupKnob(preEqLowSlider,  preEqLowLabel,  "LOW");
    setupKnob(preEqMidSlider,  preEqMidLabel,  "MID");
    setupKnob(preEqHighSlider, preEqHighLabel, "HIGH");

    preEqLowAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqLow",  preEqLowSlider);
    preEqMidAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqMid",  preEqMidSlider);
    preEqHighAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqHigh", preEqHighSlider);

    setupFreqSlider(preEqLowFreqSlider,  preEqLowFreqLabel);
    setupFreqSlider(preEqMidFreqSlider,  preEqMidFreqLabel);
    setupFreqSlider(preEqHighFreqSlider, preEqHighFreqLabel);

    preEqLowFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqLowFreq",  preEqLowFreqSlider);
    preEqMidFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqMidFreq",  preEqMidFreqSlider);
    preEqHighFreqAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqHighFreq", preEqHighFreqSlider);

    // ---- Pre-amp compressor -------------------------------------------------
    setupCompKnob(preCompThreshSlider,  preCompThreshLabel,  "THRESH");
    setupCompKnob(preCompRatioSlider,   preCompRatioLabel,   "RATIO");
    setupCompKnob(preCompAttackSlider,  preCompAttackLabel,  "ATTACK");
    setupCompKnob(preCompReleaseSlider, preCompReleaseLabel, "RELEASE");
    setupCompKnob(preCompMakeupSlider,  preCompMakeupLabel,  "MAKEUP");
    setupCompKnob(preCompBlendSlider,   preCompBlendLabel,   "BLEND");
    preCompThreshAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompThresh",   preCompThreshSlider);
    preCompRatioAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompRatio",    preCompRatioSlider);
    preCompAttackAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompAttack",   preCompAttackSlider);
    preCompReleaseAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompRelease",  preCompReleaseSlider);
    preCompMakeupAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompMakeup",   preCompMakeupSlider);
    preCompBlendAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompBlend",    preCompBlendSlider);

    // ---- Post-amp EQ ---------------------------------------------------------
    setupKnob(postEqLowSlider,  postEqLowLabel,  "LOW");
    setupKnob(postEqMidSlider,  postEqMidLabel,  "MID");
    setupKnob(postEqHighSlider, postEqHighLabel, "HIGH");

    postEqLowAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqLow",  postEqLowSlider);
    postEqMidAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqMid",  postEqMidSlider);
    postEqHighAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqHigh", postEqHighSlider);

    setupFreqSlider(postEqLowFreqSlider,  postEqLowFreqLabel);
    setupFreqSlider(postEqMidFreqSlider,  postEqMidFreqLabel);
    setupFreqSlider(postEqHighFreqSlider, postEqHighFreqLabel);

    postEqLowFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqLowFreq",  postEqLowFreqSlider);
    postEqMidFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqMidFreq",  postEqMidFreqSlider);
    postEqHighFreqAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqHighFreq", postEqHighFreqSlider);

    // ---- Post-amp compressor ------------------------------------------------
    setupCompKnob(postCompThreshSlider,  postCompThreshLabel,  "THRESH");
    setupCompKnob(postCompRatioSlider,   postCompRatioLabel,   "RATIO");
    setupCompKnob(postCompAttackSlider,  postCompAttackLabel,  "ATTACK");
    setupCompKnob(postCompReleaseSlider, postCompReleaseLabel, "RELEASE");
    setupCompKnob(postCompMakeupSlider,  postCompMakeupLabel,  "MAKEUP");
    setupCompKnob(postCompBlendSlider,   postCompBlendLabel,   "BLEND");
    postCompThreshAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompThresh",   postCompThreshSlider);
    postCompRatioAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompRatio",    postCompRatioSlider);
    postCompAttackAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompAttack",   postCompAttackSlider);
    postCompReleaseAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompRelease",  postCompReleaseSlider);
    postCompMakeupAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompMakeup",   postCompMakeupSlider);
    postCompBlendAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompBlend",    postCompBlendSlider);

    // ---- Amp knobs + neural model browser -----------------------------------
    setupLargeKnob(gainSlider,   gainLabel,   "GAIN");
    setupKnob(masterSlider,    masterLabel,    "AMP OUT");
    setupKnob(inputTrimSlider, inputTrimLabel, "INPUT GAIN");

    gainAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "inputGain",    gainSlider);
    masterAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "masterVolume", masterSlider);
    inputTrimAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "inputTrim",    inputTrimSlider);

    // Settings button (amp model loading moved here)
    styleButton(settingsBtn, false);
    settingsBtn.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible(settingsBtn);

    // ---- IR loader -----------------------------------------------------------
    // Preset combobox — populated from bundled BinaryData IRs
    {
        int id = 1;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            juce::String resName(BinaryData::namedResourceList[i]);
            if (resName.endsWith("_wav"))
            {
                juce::String displayName = resName.dropLastCharacters(4)
                                               .replaceCharacter('_', ' ');
                irPresetBox.addItem(displayName, id++);
            }
        }
    }

    irPresetBox.setColour(juce::ComboBox::backgroundColourId,  kPanel);
    irPresetBox.setColour(juce::ComboBox::textColourId,        kText);
    irPresetBox.setColour(juce::ComboBox::outlineColourId,     kBtnIdle.brighter(0.2f));
    irPresetBox.setColour(juce::ComboBox::arrowColourId,       kAccent);
    irPresetBox.setTextWhenNothingSelected("Custom IR");
    irPresetBox.onChange = [this]
    {
        const int id = irPresetBox.getSelectedId();
        if (id < 1) return;

        // Map ComboBox id back to the i-th _wav resource
        int wavIndex = 0;
        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
        {
            juce::String resName(BinaryData::namedResourceList[i]);
            if (resName.endsWith("_wav"))
            {
                ++wavIndex;
                if (wavIndex == id)
                {
                    int size = 0;
                    const char* data = BinaryData::getNamedResource(resName.toRawUTF8(), size);
                    juce::String displayName = irPresetBox.getItemText(id - 1);
                    if (audioProcessor.irLoader.loadIR(data, size, displayName))
                    {
                        irFileLabel.setText(displayName, juce::dontSendNotification);
                        irFileLabel.setColour(juce::Label::textColourId, kText);
                    }
                    break;
                }
            }
        }
    };
    addAndMakeVisible(irPresetBox);

    // Browse button (custom IR from disk)
    styleButton(loadIRBtn, false);
    loadIRBtn.onClick = [this] { loadIRFile(); };
    addAndMakeVisible(loadIRBtn);

    // IR ON toggle
    irOnBtn.setClickingTogglesState(true);
    styleButton(irOnBtn, true);
    addAndMakeVisible(irOnBtn);
    irEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "irEnabled", irOnBtn);

    // File name label
    irFileLabel.setFont(juce::Font(10.5f));
    irFileLabel.setColour(juce::Label::textColourId, kSubText);
    irFileLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(irFileLabel);

    // Reflect whatever IR is already loaded (e.g. default IR from prepareToPlay)
    if (audioProcessor.irLoader.hasLoadedIR())
    {
        irFileLabel.setText(audioProcessor.irLoader.getFileName(), juce::dontSendNotification);
        irFileLabel.setColour(juce::Label::textColourId, kText);
    }
    else
    {
        irFileLabel.setText("No IR loaded", juce::dontSendNotification);
    }
    syncIRPresetBox();

    // ---- Preset manager -------------------------------------------------------
    presetBox.setTextWhenNothingSelected("-- Select Preset --");
    presetBox.setColour(juce::ComboBox::backgroundColourId, kPanel);
    presetBox.setColour(juce::ComboBox::textColourId,       kText);
    presetBox.setColour(juce::ComboBox::outlineColourId,    kBtnIdle.brighter(0.2f));
    presetBox.setColour(juce::ComboBox::arrowColourId,      kAccent);
    presetBox.onChange = [this]
    {
        const juce::String name = presetBox.getText();
        if (name.isEmpty()) return;
        juce::File presetFile = getPresetDirectory().getChildFile(name + ".xml");
        if (!presetFile.existsAsFile()) return;

        auto xml = juce::XmlDocument::parse(presetFile);
        if (!xml) return;

        auto newState = juce::ValueTree::fromXml(*xml);
        audioProcessor.apvts.replaceState(newState);

        juce::String irPath = audioProcessor.apvts.state.getProperty("irFilePath", "");
        if (irPath.isNotEmpty() && audioProcessor.irLoader.loadIR(juce::File(irPath)))
        {
            irFileLabel.setText(audioProcessor.irLoader.getFileName(), juce::dontSendNotification);
            irFileLabel.setColour(juce::Label::textColourId, kText);
            syncIRPresetBox();
        }
        else
        {
            juce::String bundledName = audioProcessor.apvts.state.getProperty("bundledIRName", "");
            if (bundledName.isNotEmpty())
            {
                juce::String resName = bundledName.replaceCharacter(' ', '_') + "_wav";
                int irSize = 0;
                const char* irData = BinaryData::getNamedResource(resName.toRawUTF8(), irSize);
                if (irData != nullptr && audioProcessor.irLoader.loadIR(irData, irSize, bundledName))
                {
                    irFileLabel.setText(bundledName, juce::dontSendNotification);
                    irFileLabel.setColour(juce::Label::textColourId, kText);
                    syncIRPresetBox();
                }
            }
        }

        juce::String ampModelPath = audioProcessor.apvts.state.getProperty("ampModelPath", "");
        if (ampModelPath.isNotEmpty() && audioProcessor.neuralAmp.loadModel(juce::File(ampModelPath)))
        {
            modelFileLabel.setText(audioProcessor.neuralAmp.getModelFileName(), juce::dontSendNotification);
            modelFileLabel.setColour(juce::Label::textColourId, kText);
        }

        // Restore knob ranges if saved with preset
        auto rt = newState.getChildWithName("KnobRanges");
        if (rt.isValid())
        {
            for (int i = 0; i < rt.getNumChildren(); ++i)
            {
                auto e = rt.getChild(i);
                juce::String id = e.getProperty("id").toString();
                float mn = (float)e.getProperty("min");
                float mx = (float)e.getProperty("max");
                if (audioProcessor.knobRanges.ranges.count(id))
                    audioProcessor.knobRanges.ranges[id] = { mn, mx,
                        audioProcessor.knobRanges.ranges[id].skew };
            }
            rebuildAllAttachments();
        }

        // Restore section background images
        auto imagesTree = newState.getChildWithName("SectionImages");
        audioProcessor.sectionImagesTree = imagesTree.isValid()
            ? imagesTree.createCopy()
            : juce::ValueTree("SectionImages");
        loadSectionImagesFromTree();
    };
    addAndMakeVisible(presetBox);

    styleButton(savePresetBtn, false);
    savePresetBtn.onClick = [this] { saveCurrentPreset(); };
    addAndMakeVisible(savePresetBtn);

    styleButton(deletePresetBtn, false);
    deletePresetBtn.onClick = [this] { deleteCurrentPreset(); };
    addAndMakeVisible(deletePresetBtn);

    refreshPresetList();

    // ---- Post-IR 8-band EQ --------------------------------------------------
    static const char* kEqParamIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    static const char* kEqLabels[EQProcessor::kNumBands] = {
        "80","250","500","1k","2k","4k","8k","16k"
    };

    for (int b = 0; b < EQProcessor::kNumBands; ++b)
    {
        auto& s = eqSliders[b];
        s.setSliderStyle(juce::Slider::LinearVertical);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setColour(juce::Slider::trackColourId,      kAccent);
        s.setColour(juce::Slider::thumbColourId,      kAccent.brighter(0.4f));
        s.setColour(juce::Slider::backgroundColourId, kPanel);
        s.setDoubleClickReturnValue(true, 0.0);
        addAndMakeVisible(s);

        auto& l = eqLabels[b];
        l.setText(kEqLabels[b], juce::dontSendNotification);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, kSubText);
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);

        eqAtts[b] = std::make_unique<SliderAtt>(
            audioProcessor.apvts, kEqParamIds[b], s);
    }

    // ---- Output volume -------------------------------------------------------
    setupLargeKnob(outputVolSlider, outputVolLabel, "VOL");
    outputVolAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "outputVolume", outputVolSlider);

    // Apply custom knob ranges from processor state
    applyAllKnobRanges();

    // Load any section images already present in processor state (e.g. DAW project restore)
    loadSectionImagesFromTree();
}

GuitarAmpAudioProcessorEditor::~GuitarAmpAudioProcessorEditor()
{
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    // Clear LookAndFeel refs before ampLookAndFeel member is destroyed
    gainSlider.setLookAndFeel(nullptr);
    masterSlider.setLookAndFeel(nullptr);
    gateThreshSlider.setLookAndFeel(nullptr);
    preEqLowSlider.setLookAndFeel(nullptr);
    preEqMidSlider.setLookAndFeel(nullptr);
    preEqHighSlider.setLookAndFeel(nullptr);
    preCompThreshSlider.setLookAndFeel(nullptr);
    preCompRatioSlider.setLookAndFeel(nullptr);
    preCompAttackSlider.setLookAndFeel(nullptr);
    preCompReleaseSlider.setLookAndFeel(nullptr);
    preCompMakeupSlider.setLookAndFeel(nullptr);
    preCompBlendSlider.setLookAndFeel(nullptr);
    postEqLowSlider.setLookAndFeel(nullptr);
    postEqMidSlider.setLookAndFeel(nullptr);
    postEqHighSlider.setLookAndFeel(nullptr);
    postCompThreshSlider.setLookAndFeel(nullptr);
    postCompRatioSlider.setLookAndFeel(nullptr);
    postCompAttackSlider.setLookAndFeel(nullptr);
    postCompReleaseSlider.setLookAndFeel(nullptr);
    postCompMakeupSlider.setLookAndFeel(nullptr);
    postCompBlendSlider.setLookAndFeel(nullptr);
    inputTrimSlider.setLookAndFeel(nullptr);
    outputVolSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void GuitarAmpAudioProcessorEditor::setupKnob(juce::Slider& s, juce::Label& l,
                                               const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setLookAndFeel(&ampLookAndFeel);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    s.setPopupMenuEnabled(true);
    s.setColour(juce::Slider::rotarySliderFillColourId,    kAccent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kPanel.brighter(0.15f));
    s.setColour(juce::Slider::thumbColourId,               kAccent.brighter(0.3f));
    s.setColour(juce::Slider::textBoxTextColourId,         kSubText);
    s.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    s.setColour(juce::Slider::backgroundColourId,          kPanel);
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(11.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, kSubText);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void GuitarAmpAudioProcessorEditor::setupFreqSlider(juce::Slider& s, juce::Label& l)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 13);
    s.setPopupMenuEnabled(true);
    s.setColour(juce::Slider::trackColourId,             kAccent.withAlpha(0.6f));
    s.setColour(juce::Slider::thumbColourId,             kAccent);
    s.setColour(juce::Slider::backgroundColourId,        kBg);
    s.setColour(juce::Slider::textBoxTextColourId,       kSubText);
    s.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setDoubleClickReturnValue(true, 0.0);
    addAndMakeVisible(s);

    l.setText("FREQ", juce::dontSendNotification);
    l.setFont(juce::Font(9.0f));
    l.setColour(juce::Label::textColourId, kSubText.withAlpha(0.6f));
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void GuitarAmpAudioProcessorEditor::setupCompKnob(juce::Slider& s, juce::Label& l,
                                                    const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setLookAndFeel(&ampLookAndFeel);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    s.setPopupMenuEnabled(true);
    s.setColour(juce::Slider::rotarySliderFillColourId,    kGreen);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kGreen.withAlpha(0.25f));
    s.setColour(juce::Slider::thumbColourId,               kGreen.brighter(0.3f));
    s.setColour(juce::Slider::textBoxTextColourId,         kGreen.withAlpha(0.85f));
    s.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    s.setColour(juce::Slider::backgroundColourId,          kPanel);
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(11.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, kGreen.withAlpha(0.7f));
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void GuitarAmpAudioProcessorEditor::setupLargeKnob(juce::Slider& s, juce::Label& l,
                                                     const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setLookAndFeel(&ampLookAndFeel);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    s.setPopupMenuEnabled(true);
    s.setColour(juce::Slider::rotarySliderFillColourId,    kAccent);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, kAccent.withAlpha(0.35f));
    s.setColour(juce::Slider::thumbColourId,               kAccent.brighter(0.4f));
    s.setColour(juce::Slider::textBoxTextColourId,         kText);
    s.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    s.setColour(juce::Slider::backgroundColourId,          kPanel.brighter(0.08f));
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::Font(13.0f, juce::Font::bold));
    l.setColour(juce::Label::textColourId, kText);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void GuitarAmpAudioProcessorEditor::styleButton(juce::TextButton& b, bool isToggle)
{
    b.setColour(juce::TextButton::buttonColourId,   kBtnIdle);
    b.setColour(juce::TextButton::buttonOnColourId, kBtnActive);
    b.setColour(juce::TextButton::textColourOffId,  kText);
    b.setColour(juce::TextButton::textColourOnId,   juce::Colours::white);
    (void)isToggle;
}

void GuitarAmpAudioProcessorEditor::syncIRPresetBox()
{
    // The loaded IR name matches a bundled IR's display name (spaces replacing underscores)
    const juce::String loadedName = audioProcessor.irLoader.getFileName();
    for (int i = 1; i <= irPresetBox.getNumItems(); ++i)
    {
        if (irPresetBox.getItemText(i - 1) == loadedName)
        {
            irPresetBox.setSelectedId(i, juce::dontSendNotification);
            return;
        }
    }
    irPresetBox.setSelectedId(0, juce::dontSendNotification);
}

juce::File GuitarAmpAudioProcessorEditor::getPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("MF_AMP/Presets");
    dir.createDirectory();
    return dir;
}

void GuitarAmpAudioProcessorEditor::refreshPresetList()
{
    const juce::String currentName = presetBox.getText();
    presetBox.clear(juce::dontSendNotification);

    auto files = getPresetDirectory().findChildFiles(
        juce::File::findFiles, false, "*.xml");
    files.sort();

    int id = 1;
    for (const auto& f : files)
        presetBox.addItem(f.getFileNameWithoutExtension(), id++);

    // Restore selection by name
    for (int i = 1; i <= presetBox.getNumItems(); ++i)
    {
        if (presetBox.getItemText(i - 1) == currentName)
        {
            presetBox.setSelectedId(i, juce::dontSendNotification);
            return;
        }
    }
}

void GuitarAmpAudioProcessorEditor::saveCurrentPreset()
{
    auto* aw = new juce::AlertWindow("Save Preset", "Enter a name for this preset:",
                                     juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", presetBox.getText(), "Preset name:");
    aw->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true,
        juce::ModalCallbackFunction::create([this, aw](int result)
        {
            if (result == 1)
            {
                juce::String name = aw->getTextEditorContents("name").trim()
                                       .replaceCharacters("\\/:*?\"<>|", "_________");
                if (name.isEmpty()) return;

                auto state = audioProcessor.apvts.copyState();
                state.setProperty("irFilePath",
                    audioProcessor.irLoader.getFilePath(), nullptr);
                state.setProperty("bundledIRName",
                    audioProcessor.irLoader.getFilePath().isEmpty()
                        ? audioProcessor.irLoader.getFileName() : "", nullptr);
                state.setProperty("ampModelPath",
                    audioProcessor.neuralAmp.getModelFilePath(), nullptr);

                juce::ValueTree rangesTree("KnobRanges");
                for (auto& [id, kr] : audioProcessor.knobRanges.ranges)
                {
                    juce::ValueTree e("Range");
                    e.setProperty("id",  id,     nullptr);
                    e.setProperty("min", kr.min, nullptr);
                    e.setProperty("max", kr.max, nullptr);
                    rangesTree.addChild(e, -1, nullptr);
                }
                state.addChild(rangesTree, -1, nullptr);

                saveSectionImagesToTree();
                if (audioProcessor.sectionImagesTree.getNumChildren() > 0)
                    state.addChild(audioProcessor.sectionImagesTree.createCopy(), -1, nullptr);

                auto xml = state.createXml();
                xml->writeTo(getPresetDirectory().getChildFile(name + ".xml"));

                refreshPresetList();
                for (int i = 1; i <= presetBox.getNumItems(); ++i)
                    if (presetBox.getItemText(i - 1) == name)
                        { presetBox.setSelectedId(i, juce::dontSendNotification); break; }
            }
        }),
        true /* deleteWhenDismissed */);
}

void GuitarAmpAudioProcessorEditor::deleteCurrentPreset()
{
    const juce::String name = presetBox.getText();
    if (name.isEmpty()) return;

    juce::File presetFile = getPresetDirectory().getChildFile(name + ".xml");
    if (!presetFile.existsAsFile()) return;

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::WarningIcon,
        "Delete Preset",
        "Delete preset \"" + name + "\"?",
        "Delete", "Cancel", nullptr,
        juce::ModalCallbackFunction::create([this, presetFile](int result)
        {
            if (result == 1)
            {
                presetFile.deleteFile();
                refreshPresetList();
            }
        }));
}

void GuitarAmpAudioProcessorEditor::showSettingsMenu()
{
    juce::String modelName = audioProcessor.neuralAmp.hasModel()
                             ? audioProcessor.neuralAmp.getModelFileName()
                             : juce::String("No model loaded");

    juce::PopupMenu menu;
    menu.addSectionHeader("Amp Model: " + modelName);
    menu.addItem(1, "Load Amp Model...");
    menu.addSeparator();
    menu.addItem(2, "Knob Ranges...");
    menu.addItem(3, "Section Images...");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsBtn),
        [this](int result)
        {
            if (result == 1)
            {
                loadModelFile();
            }
            else if (result == 3)
            {
                showImageManagementMenu();
            }
            else if (result == 2)
            {
                auto* dialog = new KnobRangesDialog(audioProcessor,
                    [this] { rebuildAllAttachments(); });
                dialog->setSize(420, 520);

                juce::DialogWindow::LaunchOptions opts;
                opts.content.setOwned(dialog);
                opts.dialogTitle                = "Knob Ranges";
                opts.dialogBackgroundColour     = juce::Colour(0xff141414);
                opts.escapeKeyTriggersCloseButton = true;
                opts.useNativeTitleBar          = false;
                opts.resizable                  = false;
                opts.launchAsync();
            }
        });
}

void GuitarAmpAudioProcessorEditor::applyKnobRange(juce::Slider& s, const juce::String& paramId)
{
    const auto& ranges = audioProcessor.knobRanges.ranges;
    auto it = ranges.find(paramId);
    if (it == ranges.end()) return;

    const auto& kr = it->second;
    s.setNormalisableRange(juce::NormalisableRange<double>((double)kr.min, (double)kr.max, 0.0, (double)kr.skew));
    s.setValue(juce::jlimit(kr.min, kr.max, (float)s.getValue()), juce::dontSendNotification);
}

void GuitarAmpAudioProcessorEditor::applyAllKnobRanges()
{
    applyKnobRange(inputTrimSlider,      "inputTrim");
    applyKnobRange(gainSlider,           "inputGain");
    applyKnobRange(outputVolSlider,      "outputVolume");
    applyKnobRange(gateThreshSlider,     "noiseGateThreshold");
    applyKnobRange(masterSlider,         "masterVolume");
    applyKnobRange(preCompThreshSlider,  "preCompThresh");
    applyKnobRange(preCompRatioSlider,   "preCompRatio");
    applyKnobRange(preCompAttackSlider,  "preCompAttack");
    applyKnobRange(preCompReleaseSlider, "preCompRelease");
    applyKnobRange(preCompMakeupSlider,  "preCompMakeup");
    applyKnobRange(preCompBlendSlider,   "preCompBlend");
    applyKnobRange(postCompThreshSlider, "postCompThresh");
    applyKnobRange(postCompRatioSlider,  "postCompRatio");
    applyKnobRange(postCompAttackSlider, "postCompAttack");
    applyKnobRange(postCompReleaseSlider,"postCompRelease");
    applyKnobRange(postCompMakeupSlider, "postCompMakeup");
    applyKnobRange(postCompBlendSlider,  "postCompBlend");
}

void GuitarAmpAudioProcessorEditor::rebuildAllAttachments()
{
    // Destroy existing attachments before recreating
    gainAtt.reset();
    masterAtt.reset();
    gateThreshAtt.reset();
    inputTrimAtt.reset();
    outputVolAtt.reset();
    preEqLowAtt.reset();  preEqMidAtt.reset();  preEqHighAtt.reset();
    preEqLowFreqAtt.reset(); preEqMidFreqAtt.reset(); preEqHighFreqAtt.reset();
    preCompThreshAtt.reset(); preCompRatioAtt.reset();
    preCompAttackAtt.reset(); preCompReleaseAtt.reset();
    preCompMakeupAtt.reset(); preCompBlendAtt.reset();
    postEqLowAtt.reset(); postEqMidAtt.reset(); postEqHighAtt.reset();
    postEqLowFreqAtt.reset(); postEqMidFreqAtt.reset(); postEqHighFreqAtt.reset();
    postCompThreshAtt.reset(); postCompRatioAtt.reset();
    postCompAttackAtt.reset(); postCompReleaseAtt.reset();
    postCompMakeupAtt.reset(); postCompBlendAtt.reset();
    for (auto& a : eqAtts) a.reset();

    // Recreate
    gainAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "inputGain",    gainSlider);
    masterAtt    = std::make_unique<SliderAtt>(audioProcessor.apvts, "masterVolume", masterSlider);
    gateThreshAtt= std::make_unique<SliderAtt>(audioProcessor.apvts, "noiseGateThreshold", gateThreshSlider);
    inputTrimAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "inputTrim",    inputTrimSlider);
    outputVolAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "outputVolume", outputVolSlider);

    preEqLowAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqLow",  preEqLowSlider);
    preEqMidAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqMid",  preEqMidSlider);
    preEqHighAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqHigh", preEqHighSlider);
    preEqLowFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqLowFreq",  preEqLowFreqSlider);
    preEqMidFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqMidFreq",  preEqMidFreqSlider);
    preEqHighFreqAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqHighFreq", preEqHighFreqSlider);

    preCompThreshAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompThresh",   preCompThreshSlider);
    preCompRatioAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompRatio",    preCompRatioSlider);
    preCompAttackAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompAttack",   preCompAttackSlider);
    preCompReleaseAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompRelease",  preCompReleaseSlider);
    preCompMakeupAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompMakeup",   preCompMakeupSlider);
    preCompBlendAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompBlend",    preCompBlendSlider);

    postEqLowAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqLow",  postEqLowSlider);
    postEqMidAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqMid",  postEqMidSlider);
    postEqHighAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqHigh", postEqHighSlider);
    postEqLowFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqLowFreq",  postEqLowFreqSlider);
    postEqMidFreqAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqMidFreq",  postEqMidFreqSlider);
    postEqHighFreqAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqHighFreq", postEqHighFreqSlider);

    postCompThreshAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompThresh",   postCompThreshSlider);
    postCompRatioAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompRatio",    postCompRatioSlider);
    postCompAttackAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompAttack",   postCompAttackSlider);
    postCompReleaseAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompRelease",  postCompReleaseSlider);
    postCompMakeupAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompMakeup",   postCompMakeupSlider);
    postCompBlendAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompBlend",    postCompBlendSlider);

    static const char* kEqParamIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        eqAtts[b] = std::make_unique<SliderAtt>(audioProcessor.apvts, kEqParamIds[b], eqSliders[b]);

    // Re-apply custom ranges after recreating attachments
    applyAllKnobRanges();
}

void GuitarAmpAudioProcessorEditor::loadModelFile()
{
    modelFileChooser = std::make_unique<juce::FileChooser>(
        "Select RTNeural Amp Model",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.json");

    modelFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result.existsAsFile())
            {
                if (audioProcessor.neuralAmp.loadModel(result))
                {
                    modelFileLabel.setText(audioProcessor.neuralAmp.getModelFileName(),
                                           juce::dontSendNotification);
                    modelFileLabel.setColour(juce::Label::textColourId, kText);
                }
                else
                {
                    modelFileLabel.setText("Failed to load model — check file format",
                                           juce::dontSendNotification);
                    modelFileLabel.setColour(juce::Label::textColourId, kRed);
                }
            }
        });
}

void GuitarAmpAudioProcessorEditor::loadIRFile()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Cabinet IR File",
        juce::File(kIRDir),
        "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result.existsAsFile())
            {
                audioProcessor.irLoader.loadIR(result);
                irFileLabel.setText(audioProcessor.irLoader.getFileName(),
                                    juce::dontSendNotification);
                irFileLabel.setColour(juce::Label::textColourId, kText);
                syncIRPresetBox();
            }
        });
}

//==============================================================================
// Section background image helpers
//==============================================================================
juce::Rectangle<int> GuitarAmpAudioProcessorEditor::getSectionRect(int sectionId) const
{
    const int W = getWidth();
    const int H = getHeight();
    const int gap = 4, m = 8;
    const int row1Y = 60, row1H = 300;
    const int row2Y = row1Y + row1H + gap;
    const int row2H = H - row2Y - 6;

    const int wInput=80, wGate=112, wPreEQ=165, wPreComp=320, wAmp=200, wCab=195;
    int cx = m;
    const int xInput   = cx; cx += wInput + gap;
    const int xGate    = cx; cx += wGate + gap;
    const int xPreEQ   = cx; cx += wPreEQ + gap;
    const int xPreComp = cx; cx += wPreComp + gap;
    const int xAmp     = cx; cx += wAmp + gap;
    const int xCab     = cx;

    const int wPostEQ=165, wPostComp=320, wMFEQ=380;
    cx = m;
    const int xPostEQ   = cx; cx += wPostEQ + gap;
    const int xPostComp = cx; cx += wPostComp + gap;
    const int xPostIREQ = cx; cx += wMFEQ + gap;
    const int xLimiter  = cx;
    const int wLimiter  = W - m - xLimiter;

    switch (sectionId)
    {
        case kInput:    return { xInput,    row1Y, wInput,    row1H };
        case kGate:     return { xGate,     row1Y, wGate,     row1H };
        case kPreEq:    return { xPreEQ,    row1Y, wPreEQ,    row1H };
        case kPreComp:  return { xPreComp,  row1Y, wPreComp,  row1H };
        case kAmp:      return { xAmp,      row1Y, wAmp,      row1H };
        case kCabinet:  return { xCab,      row1Y, wCab,      row1H };
        case kPostEq:   return { xPostEQ,   row2Y, wPostEQ,   row2H };
        case kPostComp: return { xPostComp, row2Y, wPostComp, row2H };
        case kMfEq:     return { xPostIREQ, row2Y, wMFEQ,     row2H };
        case kOutput:   return { xLimiter,  row2Y, wLimiter,  row2H };
        case kOverallBg: return getBounds();
        default: return {};
    }
}

void GuitarAmpAudioProcessorEditor::centerImageInSection(int sectionId)
{
    auto& sd = sectionImages[sectionId];
    if (!sd.image.isValid()) return;

    auto bounds = getSectionRect(sectionId);
    float scaleX = (float)bounds.getWidth()  / (float)sd.image.getWidth();
    float scaleY = (float)bounds.getHeight() / (float)sd.image.getHeight();
    sd.scale   = juce::jmin(scaleX, scaleY);
    sd.offsetX = ((float)bounds.getWidth()  - sd.image.getWidth()  * sd.scale) * 0.5f;
    sd.offsetY = ((float)bounds.getHeight() - sd.image.getHeight() * sd.scale) * 0.5f;
}

void GuitarAmpAudioProcessorEditor::saveSectionImagesToTree()
{
    juce::ValueTree tree("SectionImages");
    for (int i = 0; i < kNumSections; ++i)
    {
        auto& sd = sectionImages[i];
        if (!sd.image.isValid()) continue;

        juce::MemoryOutputStream pngStream;
        juce::PNGImageFormat pngFmt;
        pngFmt.writeImageToStream(sd.image, pngStream);

        juce::String b64 = juce::Base64::toBase64(pngStream.getData(), pngStream.getDataSize());

        juce::ValueTree e("Image");
        e.setProperty("sectionId", i,         nullptr);
        e.setProperty("offsetX",   sd.offsetX, nullptr);
        e.setProperty("offsetY",   sd.offsetY, nullptr);
        e.setProperty("scale",     sd.scale,   nullptr);
        e.setProperty("data",      b64,        nullptr);
        tree.addChild(e, -1, nullptr);
    }
    audioProcessor.sectionImagesTree = tree;
}

void GuitarAmpAudioProcessorEditor::loadSectionImagesFromTree()
{
    for (auto& sd : sectionImages)
        sd = SectionImageData{};

    auto& tree = audioProcessor.sectionImagesTree;
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto e = tree.getChild(i);
        int id = (int)e.getProperty("sectionId", -1);
        if (id < 0 || id >= kNumSections) continue;

        juce::String b64 = e.getProperty("data", "").toString();
        if (b64.isEmpty()) continue;

        juce::MemoryOutputStream decoded;
        juce::Base64::convertFromBase64(decoded, b64);

        juce::MemoryInputStream pngStream(decoded.getData(), decoded.getDataSize(), false);
        juce::Image img = juce::ImageFileFormat::loadFrom(pngStream);
        if (!img.isValid()) continue;

        sectionImages[id].image   = img;
        sectionImages[id].offsetX = (float)e.getProperty("offsetX", 0.0);
        sectionImages[id].offsetY = (float)e.getProperty("offsetY", 0.0);
        sectionImages[id].scale   = (float)e.getProperty("scale",   1.0);
    }
    repaint();
}

void GuitarAmpAudioProcessorEditor::loadSectionImage(int sectionId)
{
    imageFileChooser = std::make_unique<juce::FileChooser>(
        "Select Background Image",
        juce::File::getSpecialLocation(juce::File::userPicturesDirectory),
        "*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.tiff;*.tif");

    imageFileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, sectionId](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (!result.existsAsFile()) return;

            juce::Image img = juce::ImageFileFormat::loadFrom(result);
            if (!img.isValid()) return;

            sectionImages[sectionId].image = img;
            centerImageInSection(sectionId);
            repaint();
            saveSectionImagesToTree();
        });
}

void GuitarAmpAudioProcessorEditor::showImageManagementMenu()
{
    static const char* kNames[] = {
        "INPUT", "NOISE GATE", "PRE EQ", "PRE COMP", "AMP", "CABINET",
        "POST EQ", "POST COMP", "MF EQ", "OUTPUT", "BACKGROUND"
    };

    juce::PopupMenu menu;
    menu.addSectionHeader("Section Images");

    for (int i = 0; i < kNumSections; ++i)
    {
        if (i == kOverallBg)
            menu.addSeparator();

        juce::PopupMenu sub;
        sub.addItem(i * 10 + 1, "Load Image...");
        if (sectionImages[i].image.isValid())
            sub.addItem(i * 10 + 2, "Remove Image");

        juce::String label(kNames[i]);
        if (sectionImages[i].image.isValid())
            label += juce::String::fromUTF8("  \xe2\x9c\x93");

        menu.addSubMenu(label, sub);
    }

    menu.addSeparator();
    menu.addItem(9999, "Clear All Images");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(settingsBtn),
        [this](int result)
        {
            if (result == 9999)
            {
                for (auto& sd : sectionImages)
                    sd = SectionImageData{};
                repaint();
                saveSectionImagesToTree();
                return;
            }

            if (result > 0)
            {
                int sectionId = result / 10;
                int action    = result % 10;

                if (sectionId >= 0 && sectionId < kNumSections)
                {
                    if (action == 1)
                        loadSectionImage(sectionId);
                    else if (action == 2)
                    {
                        sectionImages[sectionId] = SectionImageData{};
                        repaint();
                        saveSectionImagesToTree();
                    }
                }
            }
        });
}

//==============================================================================
void GuitarAmpAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBg);
    const int W = getWidth();
    const int H = getHeight();

    // Overall background image
    {
        auto& bg = sectionImages[kOverallBg];
        if (bg.image.isValid())
            g.drawImage(bg.image,
                        bg.offsetX, bg.offsetY,
                        bg.image.getWidth()  * bg.scale,
                        bg.image.getHeight() * bg.scale,
                        0, 0, bg.image.getWidth(), bg.image.getHeight());
    }

    // Title
    g.setColour(kAccent);
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("MF AMP", 10, 8, 130, 40, juce::Justification::centredLeft);

    // Header divider
    g.setColour(kPanel.brighter(0.15f));
    g.drawHorizontalLine(57, 8.0f, (float)W - 8.0f);

    // === Signal chain section boxes ===
    const int row1Y = 60, row1H = 300;
    const int row2Y = row1Y + row1H + 4;
    const int row2H = H - row2Y - 6;
    const float cr = 6.0f;
    const int gap = 4, m = 8;

    // Row 1 widths + x positions
    const int wInput=80, wGate=112, wPreEQ=165, wPreComp=320, wAmp=200, wCab=195;
    int cx = m;
    int xInput   = cx; cx += wInput + gap;
    int xGate    = cx; cx += wGate + gap;
    int xPreEQ   = cx; cx += wPreEQ + gap;
    int xPreComp = cx; cx += wPreComp + gap;
    int xAmp     = cx; cx += wAmp + gap;
    int xCab     = cx;

    // Row 2 widths + x positions
    const int wPostEQ=165, wPostComp=320, wMFEQ=380;
    cx = m;
    int xPostEQ   = cx; cx += wPostEQ + gap;
    int xPostComp = cx; cx += wPostComp + gap;
    int xPostIREQ = cx; cx += wMFEQ + gap;
    int wPostIREQ = wMFEQ;
    int xLimiter  = cx;
    int wLimiter  = W - m - xLimiter;

    struct SectionStyle { int x, y, w, h; juce::Colour border; bool comp; const char* label; };
    const SectionStyle sections[] = {
        // Row 1
        { xInput,    row1Y, wInput,    row1H, juce::Colour(0xffaa44cc), false, "INPUT"      },
        { xGate,     row1Y, wGate,     row1H, juce::Colour(0xff2277dd), false, "NOISE GATE" },
        { xPreEQ,    row1Y, wPreEQ,    row1H, kAccent,                  false, "PRE EQ"     },
        { xPreComp,  row1Y, wPreComp,  row1H, kGreen,                   true,  "PRE COMP"   },
        { xAmp,      row1Y, wAmp,      row1H, kAccent,                  false, "AMP"        },
        { xCab,      row1Y, wCab,      row1H, kAccent,                  false, "CABINET"    },
        // Row 2
        { xPostEQ,   row2Y, wPostEQ,   row2H, kAccent,                  false, "POST EQ"    },
        { xPostComp, row2Y, wPostComp, row2H, kGreen,                   true,  "POST COMP"  },
        { xPostIREQ, row2Y, wPostIREQ, row2H, kAccent,                  false, "MF EQ"      },
        { xLimiter,  row2Y, wLimiter,  row2H, kAccent,                  false, "OUTPUT"     },
    };

    for (int si = 0; si < (int)std::size(sections); ++si)
    {
        const auto& s = sections[si];
        juce::Rectangle<float> r((float)s.x, (float)s.y, (float)s.w, (float)s.h);
        // Section background image
        {
            auto& imgData = sectionImages[si];
            if (imgData.image.isValid())
            {
                juce::Graphics::ScopedSaveState saved(g);
                g.reduceClipRegion(r.toNearestInt());
                g.drawImage(imgData.image,
                            r.getX() + imgData.offsetX, r.getY() + imgData.offsetY,
                            imgData.image.getWidth()  * imgData.scale,
                            imgData.image.getHeight() * imgData.scale,
                            0, 0, imgData.image.getWidth(), imgData.image.getHeight());
            }
        }

        g.setColour(s.border.withAlpha(0.65f));
        g.drawRoundedRectangle(r.reduced(0.5f), cr, 1.3f);

        g.setColour(s.border.withAlpha(0.45f));
        g.fillRect((float)s.x + cr, (float)s.y, (float)s.w - 2.0f * cr, 2.5f);

        g.setColour(s.border.withAlpha(0.9f));
        g.setFont(juce::Font(9.5f, juce::Font::bold));
        g.drawText(s.label, s.x + 4, s.y + 5, s.w - 8, 13, juce::Justification::centred);
    }

    // Faint "MF" prefix above the GAIN knob label in the AMP section
    {
        const int gainW = 118;
        g.setColour(kSubText.withAlpha(0.35f));
        g.setFont(juce::Font(8.5f, juce::Font::plain));
        g.drawText("MF", xAmp + 6, row1Y + 19, gainW, 10, juce::Justification::centred);
    }
}

void GuitarAmpAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    // Section position constants — must match paint()
    const int gap = 4, m = 8;
    const int row1Y = 60, row1H = 300;
    const int row2Y = row1Y + row1H + gap;
    const int row2H = H - row2Y - 6;

    // Row 1 widths + x positions
    const int wInput=80, wGate=112, wPreEQ=165, wPreComp=320, wAmp=200, wCab=195;
    int cx = m;
    int xInput   = cx; cx += wInput + gap;
    int xGate    = cx; cx += wGate + gap;
    int xPreEQ   = cx; cx += wPreEQ + gap;
    int xPreComp = cx; cx += wPreComp + gap;
    int xAmp     = cx; cx += wAmp + gap;
    int xCab     = cx;

    // Row 2 widths + x positions
    const int wPostEQ=165, wPostComp=320, wMFEQ=380;
    cx = m;
    int xPostEQ   = cx; cx += wPostEQ + gap;
    int xPostComp = cx; cx += wPostComp + gap;
    int xPostIREQ = cx; cx += wMFEQ + gap;
    int wPostIREQ = wMFEQ;
    int xLimiter  = cx;
    int wLimiter  = W - m - xLimiter;

    // === Header ===
    deletePresetBtn.setBounds(W - 44,  12, 36, 26);
    savePresetBtn  .setBounds(W - 86,  12, 38, 26);
    presetBox      .setBounds(W - 220, 12, 130, 26);
    settingsBtn    .setBounds(148, 14, 80, 28);
    tunerToggle    .setBounds(234, 14, 70, 28);
    tunerDisplay   .setBounds(310,  8, W - 538, 40);

    // === INPUT TRIM (row 1) ===
    {
        const int x = xInput, w = wInput;
        inputTrimLabel .setBounds(x + 6, row1Y + 52, w - 12, 14);
        inputTrimSlider.setBounds(x + 6, row1Y + 66, w - 12, 200);
    }

    // === NOISE GATE (row 1) ===
    {
        const int x = xGate, w = wGate;
        gateEnableBtn   .setBounds(x + (w - 80) / 2, row1Y + 40, 80, 24);
        gateThreshLabel .setBounds(x + 6,             row1Y + 84, w - 12, 14);
        gateThreshSlider.setBounds(x + 6,             row1Y + 98, w - 12, 180);
    }

    // === PRE EQ (row 1) — 3 knobs + freq sliders ===
    {
        const int x    = xPreEQ, w = wPreEQ;
        const int colW = (w - 10) / 3;
        const int y0   = row1Y + 76;
        preEqLowLabel       .setBounds(x + 5,          y0,       colW, 14);
        preEqLowSlider      .setBounds(x + 5,          y0 + 14,  colW, 110);
        preEqLowFreqLabel   .setBounds(x + 5,          y0 + 128, colW, 12);
        preEqLowFreqSlider  .setBounds(x + 5,          y0 + 141, colW, 29);
        preEqMidLabel       .setBounds(x + 5 + colW,   y0,       colW, 14);
        preEqMidSlider      .setBounds(x + 5 + colW,   y0 + 14,  colW, 110);
        preEqMidFreqLabel   .setBounds(x + 5 + colW,   y0 + 128, colW, 12);
        preEqMidFreqSlider  .setBounds(x + 5 + colW,   y0 + 141, colW, 29);
        preEqHighLabel      .setBounds(x + 5 + colW*2, y0,       colW, 14);
        preEqHighSlider     .setBounds(x + 5 + colW*2, y0 + 14,  colW, 110);
        preEqHighFreqLabel  .setBounds(x + 5 + colW*2, y0 + 128, colW, 12);
        preEqHighFreqSlider .setBounds(x + 5 + colW*2, y0 + 141, colW, 29);
    }

    // === PRE COMP (row 1) — 2 rows of 3: Thresh/Ratio/Attack | Release/Makeup/Blend ===
    {
        const int x     = xPreComp, w = wPreComp;
        const int colW  = (w - 10) / 3;
        const int knobH = 100;
        const int rowH  = 14 + knobH + 6;
        const int y0    = row1Y + 42;
        preCompThreshLabel  .setBounds(x + 5,          y0,      colW, 14);
        preCompThreshSlider .setBounds(x + 5,          y0 + 14, colW, knobH);
        preCompRatioLabel   .setBounds(x + 5 + colW,   y0,      colW, 14);
        preCompRatioSlider  .setBounds(x + 5 + colW,   y0 + 14, colW, knobH);
        preCompAttackLabel  .setBounds(x + 5 + colW*2, y0,      colW, 14);
        preCompAttackSlider .setBounds(x + 5 + colW*2, y0 + 14, colW, knobH);
        const int y1 = y0 + rowH;
        preCompReleaseLabel .setBounds(x + 5,          y1,      colW, 14);
        preCompReleaseSlider.setBounds(x + 5,          y1 + 14, colW, knobH);
        preCompMakeupLabel  .setBounds(x + 5 + colW,   y1,      colW, 14);
        preCompMakeupSlider .setBounds(x + 5 + colW,   y1 + 14, colW, knobH);
        preCompBlendLabel   .setBounds(x + 5 + colW*2, y1,      colW, 14);
        preCompBlendSlider  .setBounds(x + 5 + colW*2, y1 + 14, colW, knobH);
    }

    // === AMP (row 1) ===
    {
        const int x     = xAmp;
        const int gainW = 118;
        const int rx    = x + gainW + 10;
        const int rw    = wAmp - gainW - 16;
        gainLabel .setBounds(x + 6, row1Y + 37, gainW, 14);
        gainSlider.setBounds(x + 6, row1Y + 51, gainW, 230);
        masterLabel .setBounds(rx, row1Y + 87, rw, 14);
        masterSlider.setBounds(rx, row1Y + 101, rw, 130);
    }

    // === CABINET (row 1) ===
    {
        const int x = xCab, w = wCab;
        irPresetBox.setBounds(x + 6,          row1Y + 97,  w - 12, 26);
        loadIRBtn  .setBounds(x + 6,          row1Y + 129, w - 16 - 84, 24);
        irOnBtn    .setBounds(x + w - 6 - 84, row1Y + 129, 84, 24);
        irFileLabel.setBounds(x + 6,          row1Y + 161, w - 12, 60);
    }

    // === POST EQ (row 2) — 3 knobs + freq sliders ===
    {
        const int x    = xPostEQ, w = wPostEQ;
        const int colW = (w - 10) / 3;
        const int y0   = row2Y + 66;
        postEqLowLabel       .setBounds(x + 5,          y0,       colW, 14);
        postEqLowSlider      .setBounds(x + 5,          y0 + 14,  colW, 110);
        postEqLowFreqLabel   .setBounds(x + 5,          y0 + 128, colW, 12);
        postEqLowFreqSlider  .setBounds(x + 5,          y0 + 141, colW, 29);
        postEqMidLabel       .setBounds(x + 5 + colW,   y0,       colW, 14);
        postEqMidSlider      .setBounds(x + 5 + colW,   y0 + 14,  colW, 110);
        postEqMidFreqLabel   .setBounds(x + 5 + colW,   y0 + 128, colW, 12);
        postEqMidFreqSlider  .setBounds(x + 5 + colW,   y0 + 141, colW, 29);
        postEqHighLabel      .setBounds(x + 5 + colW*2, y0,       colW, 14);
        postEqHighSlider     .setBounds(x + 5 + colW*2, y0 + 14,  colW, 110);
        postEqHighFreqLabel  .setBounds(x + 5 + colW*2, y0 + 128, colW, 12);
        postEqHighFreqSlider .setBounds(x + 5 + colW*2, y0 + 141, colW, 29);
    }

    // === POST COMP (row 2) — 2 rows of 3: Thresh/Ratio/Attack | Release/Makeup/Blend ===
    {
        const int x     = xPostComp, w = wPostComp;
        const int colW  = (w - 10) / 3;
        const int knobH = 100;
        const int rowH  = 14 + knobH + 6;
        const int y0    = row2Y + 32;
        postCompThreshLabel  .setBounds(x + 5,          y0,      colW, 14);
        postCompThreshSlider .setBounds(x + 5,          y0 + 14, colW, knobH);
        postCompRatioLabel   .setBounds(x + 5 + colW,   y0,      colW, 14);
        postCompRatioSlider  .setBounds(x + 5 + colW,   y0 + 14, colW, knobH);
        postCompAttackLabel  .setBounds(x + 5 + colW*2, y0,      colW, 14);
        postCompAttackSlider .setBounds(x + 5 + colW*2, y0 + 14, colW, knobH);
        const int y1 = y0 + rowH;
        postCompReleaseLabel .setBounds(x + 5,          y1,      colW, 14);
        postCompReleaseSlider.setBounds(x + 5,          y1 + 14, colW, knobH);
        postCompMakeupLabel  .setBounds(x + 5 + colW,   y1,      colW, 14);
        postCompMakeupSlider .setBounds(x + 5 + colW,   y1 + 14, colW, knobH);
        postCompBlendLabel   .setBounds(x + 5 + colW*2, y1,      colW, 14);
        postCompBlendSlider  .setBounds(x + 5 + colW*2, y1 + 14, colW, knobH);
    }

    // === MF EQ / POST IR EQ (row 2) — 8 vertical sliders ===
    {
        const int x      = xPostIREQ;
        const int w      = wPostIREQ;
        const int bandW  = (w - 8) / EQProcessor::kNumBands;
        const int sliderH = row2H - 50;
        for (int b = 0; b < EQProcessor::kNumBands; ++b)
        {
            const int bx = x + 4 + b * bandW;
            eqSliders[b].setBounds(bx, row2Y + 20,            bandW, sliderH);
            eqLabels [b].setBounds(bx, row2Y + 20 + sliderH,  bandW, 14);
        }
    }

    // === OUTPUT (row 2) — single large VOL knob ===
    {
        const int x = xLimiter, w = wLimiter;
        const int knobSize = juce::jmin(w - 20, row2H - 44);
        const int kx = x + (w - knobSize) / 2;
        const int ky = row2Y + (row2H - knobSize - 18) / 2 + 8;
        outputVolLabel .setBounds(kx, ky - 18, knobSize, 16);
        outputVolSlider.setBounds(kx, ky,      knobSize, knobSize);
    }
}
