#pragma once
#include <JuceHeader.h>
#include <array>

//==============================================================================
// Stores per-section border colours that the user can edit via the
// "Section Colors..." settings dialog. Colours are saved alongside the APVTS
// state in presets.
struct SectionColourSet
{
    // Indices match SectionId enum in PluginEditor.h:
    // 0=Input, 1=Gate, 2=PreEq, 3=PreComp, 4=Amp, 5=Cabinet,
    // 6=PostEq, 7=PostComp, 8=MfEq, 9=Output, 10=OverallBg
    static constexpr int kNumSections = 11;
    std::array<juce::Colour, kNumSections> colours;

    SectionColourSet() { initDefaults(); }

    void initDefaults()
    {
        const juce::Colour accent(0xffff6600);
        const juce::Colour green (0xff00dd55);

        colours[0]  = juce::Colour(0xffaa44cc); // INPUT  - purple
        colours[1]  = juce::Colour(0xff2277dd); // GATE   - blue
        colours[2]  = accent;                     // PRE EQ
        colours[3]  = green;                      // PRE COMP
        colours[4]  = accent;                     // AMP
        colours[5]  = accent;                     // CABINET
        colours[6]  = accent;                     // POST EQ
        colours[7]  = green;                      // POST COMP
        colours[8]  = accent;                     // MF EQ
        colours[9]  = accent;                     // OUTPUT
        colours[10] = juce::Colour(0xff141414);  // OVERALL BG
    }
};
