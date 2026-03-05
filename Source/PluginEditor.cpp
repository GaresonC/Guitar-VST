#include "PluginEditor.h"

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
static const int kNumPresetIRs       = 12;

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
    setSize(720, 900);

    // ---- Tuner ---------------------------------------------------------------
    addAndMakeVisible(tunerDisplay);

    tunerToggle.setClickingTogglesState(true);
    styleButton(tunerToggle, true);
    addAndMakeVisible(tunerToggle);
    tunerEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "tunerEnabled", tunerToggle);

    // ---- Noise gate ----------------------------------------------------------
    gateEnableBtn.setClickingTogglesState(true);
    styleButton(gateEnableBtn, true);
    addAndMakeVisible(gateEnableBtn);
    gateEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "noiseGateEnabled", gateEnableBtn);

    gateThreshSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gateThreshSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    gateThreshSlider.setColour(juce::Slider::trackColourId,        kAccent);
    gateThreshSlider.setColour(juce::Slider::thumbColourId,        kAccent.brighter(0.3f));
    gateThreshSlider.setColour(juce::Slider::backgroundColourId,   kPanel);
    gateThreshSlider.setColour(juce::Slider::textBoxTextColourId,  kSubText);
    gateThreshSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    gateThreshSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(gateThreshSlider);
    gateThreshAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "noiseGateThreshold", gateThreshSlider);

    gateThreshLabel.setText("THRESHOLD", juce::dontSendNotification);
    gateThreshLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    gateThreshLabel.setColour(juce::Label::textColourId, kSubText);
    gateThreshLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(gateThreshLabel);

    // ---- Pitch shifter -------------------------------------------------------
    pitchEnableBtn.setClickingTogglesState(true);
    styleButton(pitchEnableBtn, true);
    addAndMakeVisible(pitchEnableBtn);
    pitchEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "pitchShiftEnabled", pitchEnableBtn);

    pitchSemiSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchSemiSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    pitchSemiSlider.setColour(juce::Slider::trackColourId,          kAccent);
    pitchSemiSlider.setColour(juce::Slider::thumbColourId,          kAccent.brighter(0.3f));
    pitchSemiSlider.setColour(juce::Slider::backgroundColourId,     kPanel);
    pitchSemiSlider.setColour(juce::Slider::textBoxTextColourId,    kSubText);
    pitchSemiSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    pitchSemiSlider.setTextValueSuffix(" st");
    addAndMakeVisible(pitchSemiSlider);
    pitchSemiAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "pitchShiftSemitones", pitchSemiSlider);

    pitchSemiLabel.setText("SEMITONES", juce::dontSendNotification);
    pitchSemiLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    pitchSemiLabel.setColour(juce::Label::textColourId, kSubText);
    pitchSemiLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pitchSemiLabel);

    // ---- Pre-amp stage (EQ + comp) -------------------------------------------
    setupKnob(preEqLowSlider,      preEqLowLabel,      "LOW");
    setupKnob(preEqMidSlider,      preEqMidLabel,      "MID");
    setupKnob(preEqHighSlider,     preEqHighLabel,     "HIGH");
    setupKnob(preCompThreshSlider, preCompThreshLabel, "THRESH");
    setupKnob(preCompRatioSlider,  preCompRatioLabel,  "RATIO");

    preEqLowAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqLow",      preEqLowSlider);
    preEqMidAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqMid",      preEqMidSlider);
    preEqHighAtt     = std::make_unique<SliderAtt>(audioProcessor.apvts, "preEqHigh",     preEqHighSlider);
    preCompThreshAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompThresh", preCompThreshSlider);
    preCompRatioAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "preCompRatio",  preCompRatioSlider);

    // ---- Post-amp stage (EQ + comp) ------------------------------------------
    setupKnob(postEqLowSlider,      postEqLowLabel,      "LOW");
    setupKnob(postEqMidSlider,      postEqMidLabel,      "MID");
    setupKnob(postEqHighSlider,     postEqHighLabel,     "HIGH");
    setupKnob(postCompThreshSlider, postCompThreshLabel, "THRESH");
    setupKnob(postCompRatioSlider,  postCompRatioLabel,  "RATIO");

    postEqLowAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqLow",      postEqLowSlider);
    postEqMidAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqMid",      postEqMidSlider);
    postEqHighAtt     = std::make_unique<SliderAtt>(audioProcessor.apvts, "postEqHigh",     postEqHighSlider);
    postCompThreshAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompThresh", postCompThreshSlider);
    postCompRatioAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "postCompRatio",  postCompRatioSlider);

    // ---- Amp knobs -----------------------------------------------------------
    setupKnob(gainSlider,     gainLabel,     "GAIN");
    setupKnob(bassSlider,     bassLabel,     "BASS");
    setupKnob(midSlider,      midLabel,      "MID");
    setupKnob(trebleSlider,   trebleLabel,   "TREBLE");
    setupKnob(presenceSlider, presenceLabel, "PRESENCE");
    setupKnob(masterSlider,   masterLabel,   "MASTER");

    gainAtt     = std::make_unique<SliderAtt>(audioProcessor.apvts, "inputGain",    gainSlider);
    bassAtt     = std::make_unique<SliderAtt>(audioProcessor.apvts, "bass",         bassSlider);
    midAtt      = std::make_unique<SliderAtt>(audioProcessor.apvts, "mid",          midSlider);
    trebleAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "treble",       trebleSlider);
    presenceAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "presence",     presenceSlider);
    masterAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "masterVolume", masterSlider);

    // ---- IR loader -----------------------------------------------------------
    // Preset combobox
    for (int i = 1; i <= kNumPresetIRs; ++i)
        irPresetBox.addItem("Impact Studios - IR " + juce::String(i), i);

    irPresetBox.setColour(juce::ComboBox::backgroundColourId,  kPanel);
    irPresetBox.setColour(juce::ComboBox::textColourId,        kText);
    irPresetBox.setColour(juce::ComboBox::outlineColourId,     kBtnIdle.brighter(0.2f));
    irPresetBox.setColour(juce::ComboBox::arrowColourId,       kAccent);
    irPresetBox.setTextWhenNothingSelected("Custom IR");
    irPresetBox.onChange = [this]
    {
        int id = irPresetBox.getSelectedId();
        if (id >= 1 && id <= kNumPresetIRs)
        {
            juce::File irFile(kIRDir + "Impact Studios_IR " + juce::String(id) + ".wav");
            if (audioProcessor.irLoader.loadIR(irFile))
            {
                irFileLabel.setText(audioProcessor.irLoader.getFileName(),
                                    juce::dontSendNotification);
                irFileLabel.setColour(juce::Label::textColourId, kText);
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
    irFileLabel.setFont(juce::Font(13.0f));
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
}

GuitarAmpAudioProcessorEditor::~GuitarAmpAudioProcessorEditor() {}

//==============================================================================
void GuitarAmpAudioProcessorEditor::setupKnob(juce::Slider& s, juce::Label& l,
                                               const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
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
    const juce::String name = audioProcessor.irLoader.getFileName();
    for (int i = 1; i <= kNumPresetIRs; ++i)
    {
        if (name == "Impact Studios_IR " + juce::String(i) + ".wav")
        {
            irPresetBox.setSelectedId(i, juce::dontSendNotification);
            return;
        }
    }
    irPresetBox.setSelectedId(0, juce::dontSendNotification);
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
void GuitarAmpAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBg);

    // Title
    g.setColour(kAccent);
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("MF AMP", 12, 0, 220, 50, juce::Justification::centredLeft);

    // Section separator lines
    g.setColour(kPanel.brighter(0.2f));
    g.drawHorizontalLine( 50, 8.0f, (float)getWidth() - 8.0f);  // below header
    g.drawHorizontalLine(140, 8.0f, (float)getWidth() - 8.0f);  // below tuner
    g.drawHorizontalLine(200, 8.0f, (float)getWidth() - 8.0f);  // below gate
    g.drawHorizontalLine(260, 8.0f, (float)getWidth() - 8.0f);  // below pitch shift
    g.drawHorizontalLine(350, 8.0f, (float)getWidth() - 8.0f);  // below pre-amp stage
    g.drawHorizontalLine(560, 8.0f, (float)getWidth() - 8.0f);  // below tone
    g.drawHorizontalLine(660, 8.0f, (float)getWidth() - 8.0f);  // below cabinet
    g.drawHorizontalLine(750, 8.0f, (float)getWidth() - 8.0f);  // below post-amp stage

    // Section labels
    g.setColour(kSubText.withAlpha(0.6f));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("TUNER",       12,  52, 60, 14, juce::Justification::centredLeft);
    g.drawText("NOISE GATE",  12, 142, 90, 14, juce::Justification::centredLeft);
    g.drawText("PITCH SHIFT", 12, 202, 90, 14, juce::Justification::centredLeft);
    g.drawText("PRE-AMP",     12, 262, 70, 14, juce::Justification::centredLeft);
    g.drawText("TONE",        12, 352, 60, 14, juce::Justification::centredLeft);
    g.drawText("CABINET",     12, 562, 60, 14, juce::Justification::centredLeft);
    g.drawText("POST-AMP",    12, 662, 70, 14, juce::Justification::centredLeft);
    g.drawText("EQ",          12, 752, 60, 14, juce::Justification::centredLeft);
}

void GuitarAmpAudioProcessorEditor::resized()
{
    const int W = getWidth();

    // ---- Tuner section (50–140) ----------------------------------------------
    const int tunerY = 68;
    const int tunerH = 60;
    tunerToggle.setBounds(W - 80, tunerY, 70, tunerH);
    tunerDisplay.setBounds(8, tunerY, W - 96, tunerH);

    // ---- Noise gate section (140–200) ----------------------------------------
    const int gateY  = 155;
    const int gateH  = 34;
    gateEnableBtn  .setBounds(8,   gateY,  64, gateH);
    gateThreshLabel.setBounds(80,  gateY,  80, gateH);
    gateThreshSlider.setBounds(160, gateY, W - 168, gateH);

    // ---- Pitch shift section (200–260) ---------------------------------------
    const int pitchY = 215;
    const int pitchH = 34;
    pitchEnableBtn .setBounds(8,   pitchY,  64, pitchH);
    pitchSemiLabel .setBounds(80,  pitchY,  80, pitchH);
    pitchSemiSlider.setBounds(168, pitchY, W - 176, pitchH);

    // ---- Pre-amp stage (260–350) ---------------------------------------------
    const int stageKnobW = (W - 16) / 5;
    const int stageKnobH = 74;

    auto layoutStageKnob = [&](juce::Slider& s, juce::Label& l, int col, int baseY)
    {
        const int x = 8 + col * stageKnobW;
        l.setBounds(x, baseY, stageKnobW, 16);
        s.setBounds(x, baseY + 16, stageKnobW, stageKnobH);
    };

    layoutStageKnob(preEqLowSlider,      preEqLowLabel,      0, 278);
    layoutStageKnob(preEqMidSlider,      preEqMidLabel,      1, 278);
    layoutStageKnob(preEqHighSlider,     preEqHighLabel,     2, 278);
    layoutStageKnob(preCompThreshSlider, preCompThreshLabel, 3, 278);
    layoutStageKnob(preCompRatioSlider,  preCompRatioLabel,  4, 278);

    // ---- Knobs section (350–560) ---------------------------------------------
    const int knobAreaY = 368;
    const int knobAreaH = 185;
    const int knobW     = W / 6;

    auto layoutKnob = [&](juce::Slider& s, juce::Label& l, int col)
    {
        const int x = col * knobW;
        l.setBounds(x + 4, knobAreaY, knobW - 8, 16);
        s.setBounds(x + 4, knobAreaY + 16, knobW - 8, knobAreaH - 16);
    };

    layoutKnob(gainSlider,     gainLabel,     0);
    layoutKnob(bassSlider,     bassLabel,     1);
    layoutKnob(midSlider,      midLabel,      2);
    layoutKnob(trebleSlider,   trebleLabel,   3);
    layoutKnob(presenceSlider, presenceLabel, 4);
    layoutKnob(masterSlider,   masterLabel,   5);

    // ---- Cabinet section (560–660) -------------------------------------------
    const int cabY1 = 578;  // preset row
    const int cabY2 = 616;  // browse row
    const int rowH  = 30;

    irPresetBox.setBounds(8,       cabY1, W - 104, rowH);
    irOnBtn    .setBounds(W - 88,  cabY1,      80, rowH);

    loadIRBtn  .setBounds(8,       cabY2,      90, rowH);
    irFileLabel.setBounds(106,     cabY2, W - 114, rowH);

    // ---- Post-amp stage (660–750) --------------------------------------------
    layoutStageKnob(postEqLowSlider,      postEqLowLabel,      0, 678);
    layoutStageKnob(postEqMidSlider,      postEqMidLabel,      1, 678);
    layoutStageKnob(postEqHighSlider,     postEqHighLabel,     2, 678);
    layoutStageKnob(postCompThreshSlider, postCompThreshLabel, 3, 678);
    layoutStageKnob(postCompRatioSlider,  postCompRatioLabel,  4, 678);

    // ---- EQ section (750–900) ------------------------------------------------
    const int eqSectionY = 770;
    const int eqSliderH  = 100;
    const int eqLabelH   = 16;
    const int eqBandW    = (W - 16) / EQProcessor::kNumBands;

    for (int b = 0; b < EQProcessor::kNumBands; ++b)
    {
        const int eqBx = 8 + b * eqBandW;
        eqLabels [b].setBounds(eqBx, eqSectionY,            eqBandW, eqLabelH);
        eqSliders[b].setBounds(eqBx, eqSectionY + eqLabelH, eqBandW, eqSliderH);
    }
}
