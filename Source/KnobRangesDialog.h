#pragma once
#include <JuceHeader.h>
#include "KnobRangeSet.h"

class GuitarAmpAudioProcessor; // forward declaration

//==============================================================================
// Modal dialog for editing per-knob display ranges.
// Shows INPUT / GATE / AMP / PRE COMP / POST COMP sections (EQ excluded).
// Each row: label | min TextEditor | " to " | max TextEditor | unit
// Apply button writes changes to GuitarAmpAudioProcessor::knobRanges and
// triggers onApplyCallback so the editor can rebuild its slider attachments.
class KnobRangesDialog : public juce::Component
{
public:
    KnobRangesDialog(GuitarAmpAudioProcessor& proc,
                     std::function<void()> onApply);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct Row
    {
        juce::String     paramId;
        juce::String     displayName;
        juce::String     unit;
        juce::Label      nameLabel;
        juce::TextEditor minEdit;
        juce::Label      toLabel;
        juce::TextEditor maxEdit;
        juce::Label      unitLabel;
    };

    GuitarAmpAudioProcessor&    processor;
    std::function<void()>       onApplyCallback;

    juce::Component             rowContainer;
    juce::Viewport              viewport;
    juce::TextButton            applyBtn  { "Apply"  };
    juce::TextButton            cancelBtn { "Cancel" };

    std::vector<std::unique_ptr<Row>>         rows;
    std::vector<std::unique_ptr<juce::Label>> sectionLabels;

    static constexpr int kRowH  = 28;
    static constexpr int kHdrH  = 26;
    static constexpr int kNameW = 160;
    static constexpr int kEditW = 70;
    static constexpr int kToW   = 28;
    static constexpr int kUnitW = 40;
    static constexpr int kPadX  = 8;

    void buildRows();
    bool validateAndApply();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KnobRangesDialog)
};
