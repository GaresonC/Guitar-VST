#include "KnobRangesDialog.h"
#include "PluginProcessor.h"

static const juce::Colour kDlgBg     { 0xff141414 };
static const juce::Colour kDlgPanel  { 0xff1e1e1e };
static const juce::Colour kDlgAccent { 0xffff6600 };
static const juce::Colour kDlgText   { 0xffe0e0e0 };
static const juce::Colour kDlgSub    { 0xff888888 };
static const juce::Colour kDlgRed    { 0xffdd2222 };
static const juce::Colour kDlgEdit   { 0xff2a2a2a };

//==============================================================================
struct RowDef { const char* id; const char* name; const char* unit; };

static const struct SectionDef
{
    const char* title;
    RowDef rows[6];
    int numRows;
} kSections[] =
{
    { "INPUT", {
        { "inputTrim",    "Input Trim",    "dB" },
        { "inputGain",    "Input Gain",    "dB" },
        { "outputVolume", "Output Volume", "dB" },
    }, 3 },
    { "GATE", {
        { "noiseGateThreshold", "Gate Threshold", "dB" },
    }, 1 },
    { "AMP", {
        { "inputGain",    "Gain",          "dB" },
        { "masterVolume", "Amp Out",       "dB" },
    }, 2 },
    { "PRE COMP", {
        { "preCompThresh",  "Threshold", "dB" },
        { "preCompRatio",   "Ratio",     ":1" },
        { "preCompAttack",  "Attack",    "ms" },
        { "preCompRelease", "Release",   "ms" },
        { "preCompMakeup",  "Makeup",    "dB" },
        { "preCompBlend",   "Blend",     "%"  },
    }, 6 },
    { "POST COMP", {
        { "postCompThresh",  "Threshold", "dB" },
        { "postCompRatio",   "Ratio",     ":1" },
        { "postCompAttack",  "Attack",    "ms" },
        { "postCompRelease", "Release",   "ms" },
        { "postCompMakeup",  "Makeup",    "dB" },
        { "postCompBlend",   "Blend",     "%"  },
    }, 6 },
};

static constexpr int kNumSections = (int)(sizeof(kSections) / sizeof(kSections[0]));

//==============================================================================
KnobRangesDialog::KnobRangesDialog(GuitarAmpAudioProcessor& proc,
                                   std::function<void()> onApply)
    : processor(proc), onApplyCallback(std::move(onApply))
{
    buildRows();

    viewport.setViewedComponent(&rowContainer, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(8);
    viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, kDlgAccent.withAlpha(0.5f));
    addAndMakeVisible(viewport);

    auto styleBtn = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId,  juce::Colour(0xff2a2a2a));
        b.setColour(juce::TextButton::buttonOnColourId, kDlgAccent);
        b.setColour(juce::TextButton::textColourOffId,  kDlgText);
    };
    styleBtn(applyBtn);
    styleBtn(cancelBtn);

    applyBtn.onClick = [this]
    {
        if (validateAndApply())
        {
            onApplyCallback();
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->closeButtonPressed();
        }
    };

    cancelBtn.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->closeButtonPressed();
    };

    addAndMakeVisible(applyBtn);
    addAndMakeVisible(cancelBtn);
}

void KnobRangesDialog::buildRows()
{
    const auto& knobRanges = processor.knobRanges.ranges;

    for (int s = 0; s < kNumSections; ++s)
    {
        const auto& sec = kSections[s];

        sectionLabels.push_back(std::make_unique<juce::Label>());
        auto& hdr = *sectionLabels.back();
        hdr.setText(sec.title, juce::dontSendNotification);
        hdr.setFont(juce::Font(10.0f, juce::Font::bold));
        hdr.setColour(juce::Label::textColourId, kDlgAccent);
        hdr.setJustificationType(juce::Justification::centredLeft);
        rowContainer.addAndMakeVisible(hdr);

        for (int r = 0; r < sec.numRows; ++r)
        {
            const auto& rd = sec.rows[r];
            rows.push_back(std::make_unique<Row>());
            auto& row = *rows.back();
            row.paramId     = rd.id;
            row.displayName = rd.name;
            row.unit        = rd.unit;

            float curMin = -60.0f, curMax = 0.0f;
            auto it = knobRanges.find(row.paramId);
            if (it != knobRanges.end())
            {
                curMin = it->second.min;
                curMax = it->second.max;
            }

            row.nameLabel.setText(row.displayName, juce::dontSendNotification);
            row.nameLabel.setFont(juce::Font(11.5f));
            row.nameLabel.setColour(juce::Label::textColourId, kDlgText);
            row.nameLabel.setJustificationType(juce::Justification::centredLeft);
            rowContainer.addAndMakeVisible(row.nameLabel);

            auto styleEdit = [](juce::TextEditor& e, float val)
            {
                e.setColour(juce::TextEditor::backgroundColourId,    kDlgEdit);
                e.setColour(juce::TextEditor::textColourId,           kDlgText);
                e.setColour(juce::TextEditor::outlineColourId,        juce::Colour(0xff3a3a3a));
                e.setColour(juce::TextEditor::focusedOutlineColourId, kDlgAccent);
                e.setJustification(juce::Justification::centred);
                e.setText(juce::String(val, 2), false);
                e.setInputRestrictions(10, "-0123456789.");
            };
            styleEdit(row.minEdit, curMin);
            styleEdit(row.maxEdit, curMax);
            rowContainer.addAndMakeVisible(row.minEdit);
            rowContainer.addAndMakeVisible(row.maxEdit);

            row.toLabel.setText("to", juce::dontSendNotification);
            row.toLabel.setColour(juce::Label::textColourId, kDlgSub);
            row.toLabel.setJustificationType(juce::Justification::centred);
            rowContainer.addAndMakeVisible(row.toLabel);

            row.unitLabel.setText(row.unit, juce::dontSendNotification);
            row.unitLabel.setFont(juce::Font(10.5f));
            row.unitLabel.setColour(juce::Label::textColourId, kDlgSub);
            row.unitLabel.setJustificationType(juce::Justification::centredLeft);
            rowContainer.addAndMakeVisible(row.unitLabel);
        }
    }
}

void KnobRangesDialog::paint(juce::Graphics& g)
{
    g.fillAll(kDlgBg);
    g.setColour(juce::Colour(0xff333333));
    g.drawHorizontalLine(getHeight() - 48, 8.0f, (float)(getWidth() - 8));
}

void KnobRangesDialog::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    const int btnH = 32;
    const int btnY = H - btnH - 8;
    const int vpH  = btnY - 8;

    viewport .setBounds(0, 0, W, vpH);
    applyBtn .setBounds(W - 170, btnY, 76, btnH);
    cancelBtn.setBounds(W -  88, btnY, 80, btnH);

    // Layout rows inside rowContainer
    const int cW = W - viewport.getScrollBarThickness() - 2;

    int y = 6;
    int rowGlobal = 0;

    for (int s = 0; s < kNumSections; ++s)
    {
        const auto& sec = kSections[s];

        // Section header
        sectionLabels[s]->setBounds(kPadX, y, cW - kPadX * 2, kHdrH - 4);
        y += kHdrH;

        // Rows for this section
        for (int r = 0; r < sec.numRows; ++r)
        {
            auto& row = *rows[rowGlobal];
            const int mid = (kRowH - 20) / 2;
            const int ry  = y + mid;

            row.nameLabel.setBounds(kPadX,                             ry, kNameW, 20);
            row.minEdit  .setBounds(kPadX + kNameW,                    ry, kEditW, 20);
            row.toLabel  .setBounds(kPadX + kNameW + kEditW,           ry, kToW,   20);
            row.maxEdit  .setBounds(kPadX + kNameW + kEditW + kToW,    ry, kEditW, 20);
            row.unitLabel.setBounds(kPadX + kNameW + kEditW*2 + kToW,  ry, kUnitW, 20);

            y += kRowH;
            ++rowGlobal;
        }

        y += 4; // gap between sections
    }

    rowContainer.setSize(cW, y + 6);
}

bool KnobRangesDialog::validateAndApply()
{
    auto& knobRanges = processor.knobRanges.ranges;
    bool allValid = true;

    for (auto& rowPtr : rows)
    {
        auto& row = *rowPtr;
        const float mn = row.minEdit.getText().getFloatValue();
        const float mx = row.maxEdit.getText().getFloatValue();
        const bool  valid = mn < mx;

        const juce::Colour outline = valid ? juce::Colour(0xff3a3a3a) : kDlgRed;
        row.minEdit.setColour(juce::TextEditor::outlineColourId, outline);
        row.maxEdit.setColour(juce::TextEditor::outlineColourId, outline);
        row.minEdit.repaint();
        row.maxEdit.repaint();

        if (!valid)
            allValid = false;
    }

    if (!allValid) return false;

    for (auto& rowPtr : rows)
    {
        auto& row = *rowPtr;
        const float mn = row.minEdit.getText().getFloatValue();
        const float mx = row.maxEdit.getText().getFloatValue();
        if (knobRanges.count(row.paramId))
        {
            knobRanges[row.paramId].min = mn;
            knobRanges[row.paramId].max = mx;
        }
    }

    return true;
}
