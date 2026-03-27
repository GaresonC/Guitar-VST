#pragma once
#include <JuceHeader.h>
#include "SectionColourSet.h"

class GuitarAmpAudioProcessor; // forward declaration

//==============================================================================
// Modal dialog for editing per-section border colours.
// Each row: section name | clickable colour swatch (opens ColourSelector).
// Apply writes colours to GuitarAmpAudioProcessor::sectionColours and
// triggers onApplyCallback so the editor can repaint.
class SectionColoursDialog : public juce::Component
{
public:
    SectionColoursDialog(GuitarAmpAudioProcessor& proc,
                         std::function<void()> onApply);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //--------------------------------------------------------------------------
    struct ColourSwatch : public juce::Component,
                          public juce::ChangeListener
    {
        juce::Colour colour;

        explicit ColourSwatch(juce::Colour c) : colour(c) {}

        void paint(juce::Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced(1.0f);
            g.setColour(colour);
            g.fillRoundedRectangle(r, 3.0f);
            g.setColour(juce::Colours::white.withAlpha(0.25f));
            g.drawRoundedRectangle(r, 3.0f, 1.0f);
        }

        void mouseDown(const juce::MouseEvent&) override
        {
            auto* cs = new juce::ColourSelector(
                juce::ColourSelector::showColourAtTop
                | juce::ColourSelector::showSliders
                | juce::ColourSelector::showColourspace);
            cs->setSize(300, 400);
            cs->setCurrentColour(colour);
            cs->addChangeListener(this);
            juce::CallOutBox::launchAsynchronously(
                std::unique_ptr<juce::Component>(cs),
                getScreenBounds(), nullptr);
        }

        void changeListenerCallback(juce::ChangeBroadcaster* source) override
        {
            if (auto* cs = dynamic_cast<juce::ColourSelector*>(source))
            {
                colour = cs->getCurrentColour();
                repaint();
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ColourSwatch)
    };

    //--------------------------------------------------------------------------
    struct Row
    {
        int          sectionIdx;
        juce::Label  nameLabel;
        std::unique_ptr<ColourSwatch> swatch;
    };

    GuitarAmpAudioProcessor&    processor;
    std::function<void()>       onApplyCallback;

    juce::Component             rowContainer;
    juce::Viewport              viewport;
    juce::TextButton            applyBtn   { "Apply"  };
    juce::TextButton            cancelBtn  { "Cancel" };
    juce::TextButton            resetBtn   { "Reset Defaults" };

    std::vector<std::unique_ptr<Row>> rows;

    static constexpr int kRowH  = 34;
    static constexpr int kPadX  = 12;
    static constexpr int kNameW = 180;
    static constexpr int kSwatchW = 60;
    static constexpr int kSwatchH = 22;

    void buildRows();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionColoursDialog)
};
