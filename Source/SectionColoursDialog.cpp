#include "SectionColoursDialog.h"
#include "PluginProcessor.h"

static const juce::Colour kDlgBg     { 0xff141414 };
static const juce::Colour kDlgAccent { 0xffff6600 };
static const juce::Colour kDlgText   { 0xffe0e0e0 };

//==============================================================================
static const char* kSectionNames[] = {
    "Input",
    "Noise Gate",
    "Pre EQ",
    "Pre Comp",
    "Amp",
    "Cabinet",
    "Post EQ",
    "Post Comp",
    "MF EQ",
    "Output",
    "Overall Background",
};

static constexpr int kNumDisplaySections = 11; // matches SectionColourSet::kNumSections

//==============================================================================
SectionColoursDialog::SectionColoursDialog(GuitarAmpAudioProcessor& proc,
                                           std::function<void()> onApply)
    : processor(proc), onApplyCallback(std::move(onApply))
{
    buildRows();

    viewport.setViewedComponent(&rowContainer, false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(8);
    viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId,
                                               kDlgAccent.withAlpha(0.5f));
    addAndMakeVisible(viewport);

    auto styleBtn = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff2a2a2a));
        b.setColour(juce::TextButton::buttonOnColourId,  kDlgAccent);
        b.setColour(juce::TextButton::textColourOffId,   kDlgText);
    };
    styleBtn(applyBtn);
    styleBtn(cancelBtn);
    styleBtn(resetBtn);

    applyBtn.onClick = [this]
    {
        for (auto& rowPtr : rows)
            processor.sectionColours.colours[rowPtr->sectionIdx] = rowPtr->swatch->colour;

        onApplyCallback();

        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->closeButtonPressed();
    };

    cancelBtn.onClick = [this]
    {
        if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
            dw->closeButtonPressed();
    };

    resetBtn.onClick = [this]
    {
        SectionColourSet defaults;
        for (auto& rowPtr : rows)
        {
            rowPtr->swatch->colour = defaults.colours[rowPtr->sectionIdx];
            rowPtr->swatch->repaint();
        }
    };

    addAndMakeVisible(applyBtn);
    addAndMakeVisible(cancelBtn);
    addAndMakeVisible(resetBtn);
}

void SectionColoursDialog::buildRows()
{
    const auto& colours = processor.sectionColours.colours;

    for (int i = 0; i < kNumDisplaySections; ++i)
    {
        rows.push_back(std::make_unique<Row>());
        auto& row = *rows.back();
        row.sectionIdx = i;

        row.nameLabel.setText(kSectionNames[i], juce::dontSendNotification);
        row.nameLabel.setFont(juce::Font(12.0f));
        row.nameLabel.setColour(juce::Label::textColourId, kDlgText);
        row.nameLabel.setJustificationType(juce::Justification::centredLeft);
        rowContainer.addAndMakeVisible(row.nameLabel);

        row.swatch = std::make_unique<ColourSwatch>(colours[i]);
        rowContainer.addAndMakeVisible(*row.swatch);
    }
}

void SectionColoursDialog::paint(juce::Graphics& g)
{
    g.fillAll(kDlgBg);
    g.setColour(juce::Colour(0xff333333));
    g.drawHorizontalLine(getHeight() - 48, 8.0f, (float)(getWidth() - 8));
}

void SectionColoursDialog::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    const int btnH = 32;
    const int btnY = H - btnH - 8;
    const int vpH  = btnY - 8;

    viewport .setBounds(0, 0, W, vpH);
    applyBtn .setBounds(W - 170, btnY, 76, btnH);
    cancelBtn.setBounds(W -  88, btnY, 80, btnH);
    resetBtn .setBounds(kPadX,   btnY, 110, btnH);

    const int cW = W - viewport.getScrollBarThickness() - 2;

    int y = 10;
    for (auto& rowPtr : rows)
    {
        auto& row = *rowPtr;
        const int mid = (kRowH - kSwatchH) / 2;

        row.nameLabel.setBounds(kPadX, y, kNameW, kRowH);
        row.swatch->setBounds(kPadX + kNameW, y + mid, kSwatchW, kSwatchH);

        y += kRowH;
    }

    rowContainer.setSize(cW, y + 10);
}
