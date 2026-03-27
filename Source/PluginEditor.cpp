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

    // Indicator dot / needle image
    const float norm = juce::jlimit(-50.0f, 50.0f, cents) / 50.0f;
    const float ix   = mid + norm * bw * 0.45f;
    if (needleImage.isValid())
    {
        const float imgH = bh + 16.0f;
        const float scale = imgH / (float)needleImage.getHeight();
        const float imgW  = needleImage.getWidth() * scale;
        g.drawImage(needleImage,
                    ix - imgW * 0.5f, by - imgH * 0.5f + bh * 0.5f, imgW, imgH,
                    0, 0, needleImage.getWidth(), needleImage.getHeight());
    }
    else
    {
        g.setColour(ind);
        g.fillEllipse(ix - 6.0f, by - 2.0f, 12.0f, bh + 4.0f);
    }
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
    preCompRatioSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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

    // Pre-comp ratio split labels
    {
        auto setupRatioLabels = [this](juce::Label& valLbl, juce::Label& divLbl, juce::Label& oneLbl,
                                        juce::Slider& slider)
        {
            valLbl.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
            valLbl.setFont(juce::Font(11.0f));
            valLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            valLbl.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            valLbl.setJustificationType(juce::Justification::centredRight);
            valLbl.setEditable(true, true, false);
            valLbl.onEditorShow = [&valLbl] {
                if (auto* ed = valLbl.getCurrentTextEditor())
                    ed->setInputRestrictions(6, "0123456789.");
            };
            valLbl.onTextChange = [&valLbl, &slider] {
                slider.setValue(valLbl.getText().getFloatValue(), juce::sendNotification);
            };
            addAndMakeVisible(valLbl);

            divLbl.setText("/", juce::dontSendNotification);
            divLbl.setFont(juce::Font(11.0f));
            divLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            divLbl.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(divLbl);

            oneLbl.setText("1", juce::dontSendNotification);
            oneLbl.setFont(juce::Font(11.0f));
            oneLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            oneLbl.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(oneLbl);

            slider.onValueChange = [&valLbl, &slider] {
                valLbl.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
            };
        };
        setupRatioLabels(preCompRatioValueLabel, preCompRatioDivLabel, preCompRatioOneLabel,
                         preCompRatioSlider);
    }

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
    postCompRatioSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
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

    // Post-comp ratio split labels
    {
        auto setupRatioLabels = [this](juce::Label& valLbl, juce::Label& divLbl, juce::Label& oneLbl,
                                        juce::Slider& slider)
        {
            valLbl.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
            valLbl.setFont(juce::Font(11.0f));
            valLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            valLbl.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
            valLbl.setJustificationType(juce::Justification::centredRight);
            valLbl.setEditable(true, true, false);
            valLbl.onEditorShow = [&valLbl] {
                if (auto* ed = valLbl.getCurrentTextEditor())
                    ed->setInputRestrictions(6, "0123456789.");
            };
            valLbl.onTextChange = [&valLbl, &slider] {
                slider.setValue(valLbl.getText().getFloatValue(), juce::sendNotification);
            };
            addAndMakeVisible(valLbl);

            divLbl.setText("/", juce::dontSendNotification);
            divLbl.setFont(juce::Font(11.0f));
            divLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            divLbl.setJustificationType(juce::Justification::centred);
            addAndMakeVisible(divLbl);

            oneLbl.setText("1", juce::dontSendNotification);
            oneLbl.setFont(juce::Font(11.0f));
            oneLbl.setColour(juce::Label::textColourId, kGreen.withAlpha(0.85f));
            oneLbl.setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(oneLbl);

            slider.onValueChange = [&valLbl, &slider] {
                valLbl.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
            };
        };
        setupRatioLabels(postCompRatioValueLabel, postCompRatioDivLabel, postCompRatioOneLabel,
                         postCompRatioSlider);
    }

    // ---- Reverb controls -----------------------------------------------------
    setupKnob(reverbMixSlider,   reverbMixLabel,   "MIX");
    setupKnob(reverbDecaySlider, reverbDecayLabel,  "DECAY");
    setupKnob(reverbSizeSlider,  reverbSizeLabel,   "SIZE");
    reverbMixAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbMix",   reverbMixSlider);
    reverbDecayAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbDecay", reverbDecaySlider);
    reverbSizeAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbSize",  reverbSizeSlider);

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

        // Restore section colours from preset and apply to controls
        auto ct = newState.getChildWithName("SectionColours");
        if (ct.isValid())
        {
            for (int i = 0; i < ct.getNumChildren(); ++i)
            {
                auto e = ct.getChild(i);
                int idx = (int)e.getProperty("idx");
                juce::String hex = e.getProperty("hex").toString();
                if (idx >= 0 && idx < SectionColourSet::kNumSections && hex.isNotEmpty())
                    audioProcessor.sectionColours.colours[idx] = juce::Colour::fromString(hex);
            }
        }
        applySectionColours();
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

    static const char* kEqFreqParamIds[EQProcessor::kNumBands] = {
        "eq1Freq","eq2Freq","eq3Freq","eq4Freq",
        "eq5Freq","eq6Freq","eq7Freq","eq8Freq"
    };

    auto formatFreqLabel = [](float hz) -> juce::String {
        if (hz >= 1000.0f) return juce::String(hz / 1000.0f, 1) + "k";
        return juce::String(juce::roundToInt(hz));
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

        // Gain value label (above slider, editable)
        auto& vl = eqGainValueLabels[b];
        vl.setText("0.0", juce::dontSendNotification);
        vl.setFont(juce::Font(10.0f));
        vl.setColour(juce::Label::textColourId, kText);
        vl.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        vl.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
        vl.setJustificationType(juce::Justification::centred);
        vl.setEditable(true, true, false);
        vl.onEditorShow = [this, b] {
            if (auto* ed = eqGainValueLabels[b].getCurrentTextEditor())
                ed->setInputRestrictions(8, "-0123456789.");
        };
        vl.onTextChange = [this, b] {
            float val = eqGainValueLabels[b].getText().getFloatValue();
            eqSliders[b].setValue(val, juce::sendNotification);
        };
        addAndMakeVisible(vl);

        s.onValueChange = [this, b] {
            eqGainValueLabels[b].setText(
                juce::String(eqSliders[b].getValue(), 1),
                juce::dontSendNotification);
        };

        // Frequency label (below slider, editable, reflects param value)
        auto& l = eqLabels[b];
        float freqVal = audioProcessor.apvts.getRawParameterValue(kEqFreqParamIds[b])->load();
        l.setText(formatFreqLabel(freqVal), juce::dontSendNotification);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        l.setColour(juce::Label::textColourId, kSubText);
        l.setJustificationType(juce::Justification::centred);
        l.setEditable(true, true, false);
        l.onEditorShow = [this, b] {
            if (auto* ed = eqLabels[b].getCurrentTextEditor())
            {
                ed->setInputRestrictions(8, "0123456789.");
                // Show raw Hz value when editing
                float hz = audioProcessor.apvts.getRawParameterValue(
                    (juce::String("eq") + juce::String(b + 1) + "Freq").toRawUTF8())->load();
                ed->setText(juce::String(juce::roundToInt(hz)), false);
            }
        };
        l.onTextChange = [this, b, formatFreqLabel] {
            float hz = eqLabels[b].getText().getFloatValue();
            juce::String freqParamId = "eq" + juce::String(b + 1) + "Freq";
            if (auto* param = audioProcessor.apvts.getParameter(freqParamId))
                param->setValueNotifyingHost(param->convertTo0to1(hz));
            eqLabels[b].setText(formatFreqLabel(hz), juce::dontSendNotification);
        };
        addAndMakeVisible(l);

        eqAtts[b] = std::make_unique<SliderAtt>(
            audioProcessor.apvts, kEqParamIds[b], s);
    }


    // ---- Output volume -------------------------------------------------------
    setupLargeKnob(outputVolSlider, outputVolLabel, "VOL");
    outputVolAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "outputVolume", outputVolSlider);

    // ---- Section bypass buttons -----------------------------------------------
    setupBypassButton(bypassGateBtn);
    // Gate: enabled=true means NOT bypassed, so invert button colours
    bypassGateBtn.setColour(juce::TextButton::buttonColourId,   kAccent.withAlpha(0.2f));
    bypassGateBtn.setColour(juce::TextButton::buttonOnColourId, kAccent);
    gateEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "noiseGateEnabled", bypassGateBtn);

    setupBypassButton(bypassPreEqBtn);
    bypassPreEqAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassPreEq", bypassPreEqBtn);

    setupBypassButton(bypassPreCompBtn);
    bypassPreCompAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassPreComp", bypassPreCompBtn);

    setupBypassButton(bypassAmpBtn);
    bypassAmpAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassAmp", bypassAmpBtn);

    setupBypassButton(bypassCabinetBtn);
    // Cabinet: enabled=true means NOT bypassed, so invert button colours
    bypassCabinetBtn.setColour(juce::TextButton::buttonColourId,   kAccent.withAlpha(0.2f));
    bypassCabinetBtn.setColour(juce::TextButton::buttonOnColourId, kAccent);
    irEnabledAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "irEnabled", bypassCabinetBtn);

    setupBypassButton(bypassPostEqBtn);
    bypassPostEqAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassPostEq", bypassPostEqBtn);

    setupBypassButton(bypassPostCompBtn);
    bypassPostCompAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassPostComp", bypassPostCompBtn);

    setupBypassButton(bypassMfEqBtn);
    bypassMfEqAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassMfEq", bypassMfEqBtn);

    setupBypassButton(bypassReverbBtn);
    bypassReverbAtt = std::make_unique<ButtonAtt>(audioProcessor.apvts, "bypassReverb", bypassReverbBtn);

    // Apply custom knob ranges from processor state
    applyAllKnobRanges();

    // Load any section images already present in processor state (e.g. DAW project restore)
    loadSectionImagesFromTree();

    // Apply section colours to all controls (knobs, sliders, labels, buttons, combos)
    applySectionColours();
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
    reverbMixSlider.setLookAndFeel(nullptr);
    reverbDecaySlider.setLookAndFeel(nullptr);
    reverbSizeSlider.setLookAndFeel(nullptr);
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

void GuitarAmpAudioProcessorEditor::setupBypassButton(juce::TextButton& btn)
{
    btn.setClickingTogglesState(true);
    btn.setButtonText("");
    btn.setTooltip("bypass");
    btn.setColour(juce::TextButton::buttonColourId,   kAccent);
    btn.setColour(juce::TextButton::buttonOnColourId,  kAccent.withAlpha(0.2f));
    btn.setColour(juce::TextButton::textColourOffId,   juce::Colours::transparentBlack);
    btn.setColour(juce::TextButton::textColourOnId,    juce::Colours::transparentBlack);
    btn.onClick = [this] { updateBypassVisuals(); };
    addAndMakeVisible(btn);
}

void GuitarAmpAudioProcessorEditor::updateBypassVisuals()
{
    // Gate bypass is inverted: enabled=true means NOT bypassed
    bool gateBypassed = !bypassGateBtn.getToggleState();
    gateThreshSlider.setAlpha(gateBypassed ? 0.2f : 1.0f);
    gateThreshLabel .setAlpha(gateBypassed ? 0.2f : 1.0f);

    auto setAlpha = [](float a, auto&... comps) { (comps.setAlpha(a), ...); };

    bool preEqBypassed = bypassPreEqBtn.getToggleState();
    setAlpha(preEqBypassed ? 0.2f : 1.0f,
        preEqLowSlider, preEqMidSlider, preEqHighSlider,
        preEqLowLabel, preEqMidLabel, preEqHighLabel,
        preEqLowFreqSlider, preEqMidFreqSlider, preEqHighFreqSlider,
        preEqLowFreqLabel, preEqMidFreqLabel, preEqHighFreqLabel);

    bool preCompBypassed = bypassPreCompBtn.getToggleState();
    setAlpha(preCompBypassed ? 0.2f : 1.0f,
        preCompThreshSlider, preCompRatioSlider, preCompAttackSlider,
        preCompReleaseSlider, preCompMakeupSlider, preCompBlendSlider,
        preCompThreshLabel, preCompRatioLabel, preCompAttackLabel,
        preCompReleaseLabel, preCompMakeupLabel, preCompBlendLabel,
        preCompRatioValueLabel, preCompRatioDivLabel, preCompRatioOneLabel);

    bool ampBypassed = bypassAmpBtn.getToggleState();
    setAlpha(ampBypassed ? 0.2f : 1.0f,
        gainSlider, masterSlider, gainLabel, masterLabel,
        loadModelBtn, modelFileLabel);

    // Cabinet bypass is inverted: enabled=true means NOT bypassed
    bool cabBypassed = !bypassCabinetBtn.getToggleState();
    setAlpha(cabBypassed ? 0.2f : 1.0f,
        irPresetBox, loadIRBtn, irFileLabel);

    bool postEqBypassed = bypassPostEqBtn.getToggleState();
    setAlpha(postEqBypassed ? 0.2f : 1.0f,
        postEqLowSlider, postEqMidSlider, postEqHighSlider,
        postEqLowLabel, postEqMidLabel, postEqHighLabel,
        postEqLowFreqSlider, postEqMidFreqSlider, postEqHighFreqSlider,
        postEqLowFreqLabel, postEqMidFreqLabel, postEqHighFreqLabel);

    bool postCompBypassed = bypassPostCompBtn.getToggleState();
    setAlpha(postCompBypassed ? 0.2f : 1.0f,
        postCompThreshSlider, postCompRatioSlider, postCompAttackSlider,
        postCompReleaseSlider, postCompMakeupSlider, postCompBlendSlider,
        postCompThreshLabel, postCompRatioLabel, postCompAttackLabel,
        postCompReleaseLabel, postCompMakeupLabel, postCompBlendLabel,
        postCompRatioValueLabel, postCompRatioDivLabel, postCompRatioOneLabel);

    bool reverbBypassed = bypassReverbBtn.getToggleState();
    setAlpha(reverbBypassed ? 0.2f : 1.0f,
        reverbMixSlider, reverbDecaySlider, reverbSizeSlider,
        reverbMixLabel, reverbDecayLabel, reverbSizeLabel);

    bool mfEqBypassed = bypassMfEqBtn.getToggleState();
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
    {
        eqSliders[b]        .setAlpha(mfEqBypassed ? 0.2f : 1.0f);
        eqLabels[b]         .setAlpha(mfEqBypassed ? 0.2f : 1.0f);
        eqGainValueLabels[b].setAlpha(mfEqBypassed ? 0.2f : 1.0f);
    }
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

                for (auto c = state.getChildWithName("KnobRanges"); c.isValid();
                     c = state.getChildWithName("KnobRanges"))
                    state.removeChild(c, nullptr);
                for (auto c = state.getChildWithName("SectionImages"); c.isValid();
                     c = state.getChildWithName("SectionImages"))
                    state.removeChild(c, nullptr);

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

                // Save section colours
                for (auto c = state.getChildWithName("SectionColours"); c.isValid();
                     c = state.getChildWithName("SectionColours"))
                    state.removeChild(c, nullptr);

                juce::ValueTree coloursTree("SectionColours");
                for (int i = 0; i < SectionColourSet::kNumSections; ++i)
                {
                    juce::ValueTree e("Colour");
                    e.setProperty("idx", i, nullptr);
                    e.setProperty("hex", audioProcessor.sectionColours.colours[i].toDisplayString(true), nullptr);
                    coloursTree.addChild(e, -1, nullptr);
                }
                state.addChild(coloursTree, -1, nullptr);

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
    menu.addItem(4, "Section Colors...");

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
                dialog->setSize(420, 600);

                juce::DialogWindow::LaunchOptions opts;
                opts.content.setOwned(dialog);
                opts.dialogTitle                = "Knob Ranges";
                opts.dialogBackgroundColour     = juce::Colour(0xff141414);
                opts.escapeKeyTriggersCloseButton = true;
                opts.useNativeTitleBar          = false;
                opts.resizable                  = false;
                opts.launchAsync();
            }
            else if (result == 4)
            {
                auto* dialog = new SectionColoursDialog(audioProcessor,
                    [this] { applySectionColours(); });
                dialog->setSize(380, 480);

                juce::DialogWindow::LaunchOptions opts;
                opts.content.setOwned(dialog);
                opts.dialogTitle                = "Section Colors";
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
    applyKnobRange(reverbMixSlider,     "reverbMix");
    applyKnobRange(reverbDecaySlider,   "reverbDecay");
    applyKnobRange(reverbSizeSlider,    "reverbSize");

    static const char* kEqIds[] = { "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
                                     "eq5Gain","eq6Gain","eq7Gain","eq8Gain" };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        applyKnobRange(eqSliders[b], kEqIds[b]);
}

void GuitarAmpAudioProcessorEditor::applySectionColours()
{
    const auto& sc = audioProcessor.sectionColours.colours;

    auto tintKnob = [](juce::Slider& s, juce::Label& l, juce::Colour c)
    {
        s.setColour(juce::Slider::rotarySliderFillColourId,    c);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, c.withAlpha(0.25f));
        s.setColour(juce::Slider::thumbColourId,               c.brighter(0.3f));
        s.setColour(juce::Slider::textBoxTextColourId,         c.withAlpha(0.85f));
        l.setColour(juce::Label::textColourId,                 c.withAlpha(0.7f));
    };

    auto tintLargeKnob = [](juce::Slider& s, juce::Label& l, juce::Colour c)
    {
        s.setColour(juce::Slider::rotarySliderFillColourId,    c);
        s.setColour(juce::Slider::rotarySliderOutlineColourId, c.withAlpha(0.35f));
        s.setColour(juce::Slider::thumbColourId,               c.brighter(0.4f));
        s.setColour(juce::Slider::textBoxTextColourId,         c.withAlpha(0.85f));
        l.setColour(juce::Label::textColourId,                 c.withAlpha(0.9f));
    };

    auto tintFreqSlider = [](juce::Slider& s, juce::Label& l, juce::Colour c)
    {
        s.setColour(juce::Slider::trackColourId, c.withAlpha(0.6f));
        s.setColour(juce::Slider::thumbColourId, c);
        (void)l;
    };

    auto tintEqSlider = [](juce::Slider& s, juce::Label& /*l*/, juce::Colour c)
    {
        s.setColour(juce::Slider::trackColourId, c);
        s.setColour(juce::Slider::thumbColourId, c.brighter(0.4f));
    };

    auto tintButton = [](juce::TextButton& b, juce::Colour c)
    {
        b.setColour(juce::TextButton::buttonColourId,   c);
        b.setColour(juce::TextButton::buttonOnColourId, c.withAlpha(0.2f));
    };

    auto tintInvertedButton = [](juce::TextButton& b, juce::Colour c)
    {
        b.setColour(juce::TextButton::buttonColourId,   c.withAlpha(0.2f));
        b.setColour(juce::TextButton::buttonOnColourId, c);
    };

    auto tintComboBox = [](juce::ComboBox& cb, juce::Colour c)
    {
        cb.setColour(juce::ComboBox::arrowColourId, c);
    };

    // kInput
    tintKnob(inputTrimSlider, inputTrimLabel, sc[kInput]);

    // kGate
    tintLargeKnob(gateThreshSlider, gateThreshLabel, sc[kGate]);
    tintInvertedButton(bypassGateBtn, sc[kGate]);

    // kPreEq
    {
        auto c = sc[kPreEq];
        tintKnob(preEqLowSlider,  preEqLowLabel,  c);
        tintKnob(preEqMidSlider,  preEqMidLabel,  c);
        tintKnob(preEqHighSlider, preEqHighLabel, c);
        tintFreqSlider(preEqLowFreqSlider,  preEqLowFreqLabel,  c);
        tintFreqSlider(preEqMidFreqSlider,  preEqMidFreqLabel,  c);
        tintFreqSlider(preEqHighFreqSlider, preEqHighFreqLabel, c);
        tintButton(bypassPreEqBtn, c);
    }

    // kPreComp
    {
        auto c = sc[kPreComp];
        tintKnob(preCompThreshSlider,  preCompThreshLabel,  c);
        tintKnob(preCompRatioSlider,   preCompRatioLabel,   c);
        tintKnob(preCompAttackSlider,  preCompAttackLabel,  c);
        tintKnob(preCompReleaseSlider, preCompReleaseLabel, c);
        tintKnob(preCompMakeupSlider,  preCompMakeupLabel,  c);
        tintKnob(preCompBlendSlider,   preCompBlendLabel,   c);
        tintButton(bypassPreCompBtn, c);
    }

    // kAmp
    tintLargeKnob(gainSlider, gainLabel, sc[kAmp]);
    tintKnob(masterSlider, masterLabel, sc[kAmp]);
    tintButton(bypassAmpBtn, sc[kAmp]);

    // kCabinet
    tintComboBox(irPresetBox, sc[kCabinet]);
    tintButton(loadIRBtn, sc[kCabinet]);
    tintInvertedButton(bypassCabinetBtn, sc[kCabinet]);

    // kPostEq
    {
        auto c = sc[kPostEq];
        tintKnob(postEqLowSlider,  postEqLowLabel,  c);
        tintKnob(postEqMidSlider,  postEqMidLabel,  c);
        tintKnob(postEqHighSlider, postEqHighLabel, c);
        tintFreqSlider(postEqLowFreqSlider,  postEqLowFreqLabel,  c);
        tintFreqSlider(postEqMidFreqSlider,  postEqMidFreqLabel,  c);
        tintFreqSlider(postEqHighFreqSlider, postEqHighFreqLabel, c);
        tintButton(bypassPostEqBtn, c);
    }

    // kPostComp
    {
        auto c = sc[kPostComp];
        tintKnob(postCompThreshSlider,  postCompThreshLabel,  c);
        tintKnob(postCompRatioSlider,   postCompRatioLabel,   c);
        tintKnob(postCompAttackSlider,  postCompAttackLabel,  c);
        tintKnob(postCompReleaseSlider, postCompReleaseLabel, c);
        tintKnob(postCompMakeupSlider,  postCompMakeupLabel,  c);
        tintKnob(postCompBlendSlider,   postCompBlendLabel,   c);
        tintButton(bypassPostCompBtn, c);
    }

    // kReverb
    {
        auto c = sc[kReverb];
        tintKnob(reverbMixSlider,   reverbMixLabel,   c);
        tintKnob(reverbDecaySlider, reverbDecayLabel,  c);
        tintKnob(reverbSizeSlider,  reverbSizeLabel,   c);
        tintButton(bypassReverbBtn, c);
    }

    // kMfEq
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
    {
        tintEqSlider(eqSliders[b], eqLabels[b], sc[kMfEq]);
        eqGainValueLabels[b].setColour(juce::Label::textColourId, sc[kMfEq].withAlpha(0.85f));
    }
    tintButton(bypassMfEqBtn, sc[kMfEq]);

    // kOutput
    tintLargeKnob(outputVolSlider, outputVolLabel, sc[kOutput]);

    repaint();
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
    reverbMixAtt.reset(); reverbDecayAtt.reset(); reverbSizeAtt.reset();
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

    reverbMixAtt   = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbMix",   reverbMixSlider);
    reverbDecayAtt = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbDecay", reverbDecaySlider);
    reverbSizeAtt  = std::make_unique<SliderAtt>(audioProcessor.apvts, "reverbSize",  reverbSizeSlider);

    static const char* kEqParamIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        eqAtts[b] = std::make_unique<SliderAtt>(audioProcessor.apvts, kEqParamIds[b], eqSliders[b]);

    // Re-apply custom ranges after recreating attachments
    applyAllKnobRanges();

    // Refresh EQ gain and frequency labels
    auto fmtFreq = [](float hz) -> juce::String {
        if (hz >= 1000.0f) return juce::String(hz / 1000.0f, 1) + "k";
        return juce::String(juce::roundToInt(hz));
    };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
    {
        eqGainValueLabels[b].setText(
            juce::String(eqSliders[b].getValue(), 1), juce::dontSendNotification);
        float hz = audioProcessor.apvts.getRawParameterValue(
            (juce::String("eq") + juce::String(b + 1) + "Freq").toRawUTF8())->load();
        eqLabels[b].setText(fmtFreq(hz), juce::dontSendNotification);
    }
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

    // Row 2: POST COMP | REVERB | MF EQ | OUTPUT
    const int wPostComp=320, wReverb=165, wMFEQ=380;
    cx = m;
    const int xPostComp = cx; cx += wPostComp + gap;
    const int xReverb   = cx; cx += wReverb + gap;
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
        case kPostEq:   return { xAmp,      row1Y, wAmp,      row1H }; // POST EQ controls live in AMP section now
        case kPostComp: return { xPostComp, row2Y, wPostComp, row2H };
        case kReverb:   return { xReverb,   row2Y, wReverb,   row2H };
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
    // Save tuner needle image
    if (tunerDisplay.needleImage.isValid())
    {
        juce::MemoryOutputStream pngStream;
        juce::PNGImageFormat pngFmt;
        pngFmt.writeImageToStream(tunerDisplay.needleImage, pngStream);
        juce::String b64 = juce::Base64::toBase64(pngStream.getData(), pngStream.getDataSize());
        juce::ValueTree e("TunerNeedle");
        e.setProperty("data", b64, nullptr);
        tree.addChild(e, -1, nullptr);
    }

    audioProcessor.sectionImagesTree = tree;
}

void GuitarAmpAudioProcessorEditor::loadSectionImagesFromTree()
{
    for (auto& sd : sectionImages)
        sd = SectionImageData{};
    tunerDisplay.needleImage = juce::Image{};

    auto& tree = audioProcessor.sectionImagesTree;
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto e = tree.getChild(i);

        // Load tuner needle
        if (e.hasType("TunerNeedle"))
        {
            juce::String b64 = e.getProperty("data", "").toString();
            if (b64.isNotEmpty())
            {
                juce::MemoryOutputStream decoded;
                juce::Base64::convertFromBase64(decoded, b64);
                juce::MemoryInputStream pngStream(decoded.getData(), decoded.getDataSize(), false);
                tunerDisplay.needleImage = juce::ImageFileFormat::loadFrom(pngStream);
            }
            continue;
        }

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
        "POST EQ", "POST COMP", "REVERB", "MF EQ", "OUTPUT", "BACKGROUND"
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

    // Tuner needle image option
    {
        juce::PopupMenu needleSub;
        needleSub.addItem(9901, "Load Image...");
        if (tunerDisplay.needleImage.isValid())
            needleSub.addItem(9902, "Remove Image");
        juce::String needleLabel = "TUNER NEEDLE";
        if (tunerDisplay.needleImage.isValid())
            needleLabel += juce::String::fromUTF8("  \xe2\x9c\x93");
        menu.addSubMenu(needleLabel, needleSub);
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
                tunerDisplay.needleImage = juce::Image{};
                repaint();
                saveSectionImagesToTree();
                return;
            }

            if (result == 9901)
            {
                // Load tuner needle image
                imageFileChooser = std::make_unique<juce::FileChooser>(
                    "Select Tuner Needle Image",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.png;*.jpg;*.jpeg;*.gif;*.bmp;*.tiff;*.tif");
                imageFileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this](const juce::FileChooser& fc)
                    {
                        auto result = fc.getResult();
                        if (result.existsAsFile())
                        {
                            tunerDisplay.needleImage = juce::ImageFileFormat::loadFrom(result);
                            repaint();
                            saveSectionImagesToTree();
                        }
                    });
                return;
            }

            if (result == 9902)
            {
                tunerDisplay.needleImage = juce::Image{};
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

    // Row 2 widths + x positions: POST COMP | REVERB | MF EQ | OUTPUT
    const int wPostComp=320, wReverb=165, wMFEQ=380;
    cx = m;
    int xPostComp = cx; cx += wPostComp + gap;
    int xReverb   = cx; cx += wReverb + gap;
    int xPostIREQ = cx; cx += wMFEQ + gap;
    int wPostIREQ = wMFEQ;
    int xLimiter  = cx;
    int wLimiter  = W - m - xLimiter;

    struct SectionStyle { int x, y, w, h; juce::Colour border; bool comp; const char* label; int sectionId; };
    const auto& sc = audioProcessor.sectionColours.colours;
    const SectionStyle sections[] = {
        // Row 1
        { xInput,    row1Y, wInput,    row1H, sc[kInput],    false, "INPUT",      kInput    },
        { xGate,     row1Y, wGate,     row1H, sc[kGate],     false, "NOISE GATE", kGate     },
        { xPreEQ,    row1Y, wPreEQ,    row1H, sc[kPreEq],    false, "PRE EQ",     kPreEq    },
        { xPreComp,  row1Y, wPreComp,  row1H, sc[kPreComp],  true,  "PRE COMP",   kPreComp  },
        { xAmp,      row1Y, wAmp,      row1H, sc[kAmp],      false, "AMP",        kAmp      },
        { xCab,      row1Y, wCab,      row1H, sc[kCabinet],  false, "CABINET",    kCabinet  },
        // Row 2
        { xPostComp, row2Y, wPostComp, row2H, sc[kPostComp], true,  "POST COMP",  kPostComp },
        { xReverb,   row2Y, wReverb,   row2H, sc[kReverb],   false, "REVERB",     kReverb   },
        { xPostIREQ, row2Y, wPostIREQ, row2H, sc[kMfEq],     false, "MF EQ",      kMfEq     },
        { xLimiter,  row2Y, wLimiter,  row2H, sc[kOutput],   false, "OUTPUT",     kOutput   },
    };

    for (int si = 0; si < (int)std::size(sections); ++si)
    {
        const auto& s = sections[si];
        juce::Rectangle<float> r((float)s.x, (float)s.y, (float)s.w, (float)s.h);
        // Section background image
        {
            auto& imgData = sectionImages[s.sectionId];
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

    // Row 2 widths + x positions: POST COMP | REVERB | MF EQ | OUTPUT
    const int wPostComp=320, wReverb=165, wMFEQ=380;
    cx = m;
    int xPostComp = cx; cx += wPostComp + gap;
    int xReverb   = cx; cx += wReverb + gap;
    int xPostIREQ = cx; cx += wMFEQ + gap;
    int wPostIREQ = wMFEQ;
    int xLimiter  = cx;
    int wLimiter  = W - m - xLimiter;

    // === Header ===
    savePresetBtn  .setBounds(W - 48,   2, 40, 18);
    deletePresetBtn.setBounds(W - 48,  22, 40, 18);
    presetBox      .setBounds(W - 200, 12, 148, 26);
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
        gateThreshLabel .setBounds(x + 6,             row1Y + 52, w - 12, 14);
        gateThreshSlider.setBounds(x + 6,             row1Y + 66, w - 12, 200);
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
        {
            const int ry = y0 + 14 + knobH;
            const int rx = x + 5 + colW;
            const int tw = colW / 3;
            preCompRatioValueLabel.setBounds(rx,          ry, tw + 6, 16);
            preCompRatioDivLabel  .setBounds(rx + tw + 6, ry, 10,     16);
            preCompRatioOneLabel  .setBounds(rx + tw + 16,ry, tw - 8, 16);
        }
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

    // === AMP (row 1) — Gain + AMP OUT on top, POST EQ knobs on bottom ===
    {
        const int x = xAmp, w = wAmp;
        // Top half: GAIN (medium) + AMP OUT side by side
        const int gainW = 90;
        const int masterW = w - gainW - 16;
        const int topH = 140;
        gainLabel .setBounds(x + 6,            row1Y + 25, gainW,  14);
        gainSlider.setBounds(x + 6,            row1Y + 39, gainW,  topH);
        masterLabel .setBounds(x + gainW + 10, row1Y + 25, masterW, 14);
        masterSlider.setBounds(x + gainW + 10, row1Y + 39, masterW, topH - 20);

        // Bottom half: 3 POST EQ knobs + freq sliders
        const int eqY0 = row1Y + 39 + topH + 4;
        const int colW = (w - 10) / 3;
        const int knobH = 65;
        postEqLowLabel       .setBounds(x + 5,          eqY0,            colW, 12);
        postEqLowSlider      .setBounds(x + 5,          eqY0 + 12,      colW, knobH);
        postEqLowFreqLabel   .setBounds(x + 5,          eqY0 + 12 + knobH, colW, 10);
        postEqLowFreqSlider  .setBounds(x + 5,          eqY0 + 22 + knobH, colW, 22);
        postEqMidLabel       .setBounds(x + 5 + colW,   eqY0,            colW, 12);
        postEqMidSlider      .setBounds(x + 5 + colW,   eqY0 + 12,      colW, knobH);
        postEqMidFreqLabel   .setBounds(x + 5 + colW,   eqY0 + 12 + knobH, colW, 10);
        postEqMidFreqSlider  .setBounds(x + 5 + colW,   eqY0 + 22 + knobH, colW, 22);
        postEqHighLabel      .setBounds(x + 5 + colW*2, eqY0,            colW, 12);
        postEqHighSlider     .setBounds(x + 5 + colW*2, eqY0 + 12,      colW, knobH);
        postEqHighFreqLabel  .setBounds(x + 5 + colW*2, eqY0 + 12 + knobH, colW, 10);
        postEqHighFreqSlider .setBounds(x + 5 + colW*2, eqY0 + 22 + knobH, colW, 22);
    }

    // === CABINET (row 1) ===
    {
        const int x = xCab, w = wCab;
        irPresetBox.setBounds(x + 6,          row1Y + 97,  w - 12, 26);
        loadIRBtn  .setBounds(x + 6,          row1Y + 129, w - 12, 24);
        irFileLabel.setBounds(x + 6,          row1Y + 161, w - 12, 60);
    }

    // === REVERB (row 2) — 3 knobs: Mix, Decay, Size ===
    {
        const int x    = xReverb, w = wReverb;
        const int colW = (w - 10) / 3;
        const int y0   = row2Y + 66;
        reverbMixLabel  .setBounds(x + 5,          y0,       colW, 14);
        reverbMixSlider .setBounds(x + 5,          y0 + 14,  colW, 110);
        reverbDecayLabel .setBounds(x + 5 + colW,  y0,       colW, 14);
        reverbDecaySlider.setBounds(x + 5 + colW,  y0 + 14,  colW, 110);
        reverbSizeLabel  .setBounds(x + 5 + colW*2, y0,      colW, 14);
        reverbSizeSlider .setBounds(x + 5 + colW*2, y0 + 14, colW, 110);
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
        {
            const int ry = y0 + 14 + knobH;
            const int rx = x + 5 + colW;
            const int tw = colW / 3;
            postCompRatioValueLabel.setBounds(rx,          ry, tw + 6, 16);
            postCompRatioDivLabel  .setBounds(rx + tw + 6, ry, 10,     16);
            postCompRatioOneLabel  .setBounds(rx + tw + 16,ry, tw - 8, 16);
        }
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
        const int gainLblH = 16;
        const int freqLblH = 14;
        const int sliderH  = row2H - 20 - gainLblH - freqLblH - 4;
        for (int b = 0; b < EQProcessor::kNumBands; ++b)
        {
            const int bx = x + 4 + b * bandW;
            eqGainValueLabels[b].setBounds(bx, row2Y + 20,                        bandW, gainLblH);
            eqSliders[b]       .setBounds(bx, row2Y + 20 + gainLblH,             bandW, sliderH);
            eqLabels[b]        .setBounds(bx, row2Y + 20 + gainLblH + sliderH,   bandW, freqLblH);
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

    // === Bypass buttons (bottom-center of each section) ===
    {
        const int bw = 24, bh = 14;
        auto placeBypass = [&](juce::TextButton& btn, int sx, int sy, int sw, int sh)
        {
            btn.setBounds(sx + (sw - bw) / 2, sy + sh - bh - 4, bw, bh);
        };
        placeBypass(bypassGateBtn,     xGate,     row1Y, wGate,     row1H);
        placeBypass(bypassPreEqBtn,    xPreEQ,    row1Y, wPreEQ,    row1H);
        placeBypass(bypassPreCompBtn,  xPreComp,  row1Y, wPreComp,  row1H);
        placeBypass(bypassAmpBtn,      xAmp,      row1Y, wAmp,      row1H);
        placeBypass(bypassCabinetBtn,  xCab,      row1Y, wCab,      row1H);
        // POST EQ bypass — placed to the right of AMP bypass at bottom of AMP section
        bypassPostEqBtn.setBounds(xAmp + (wAmp - bw) / 2 + bw + 4, row1Y + row1H - bh - 4, bw, bh);
        placeBypass(bypassPostCompBtn, xPostComp, row2Y, wPostComp, row2H);
        placeBypass(bypassReverbBtn,   xReverb,   row2Y, wReverb,   row2H);
        placeBypass(bypassMfEqBtn,     xPostIREQ, row2Y, wPostIREQ, row2H);
    }

    updateBypassVisuals();
}
