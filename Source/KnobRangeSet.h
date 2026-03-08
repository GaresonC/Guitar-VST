#pragma once
#include <JuceHeader.h>
#include <map>

//==============================================================================
// Stores per-parameter display ranges that the user can edit via the
// "Knob Ranges..." settings dialog. Ranges are saved alongside the APVTS
// state in presets.  EQ parameters (preEq*, postEq*, eq*Gain) are excluded.
struct KnobRange
{
    float min;
    float max;
    float skew = 1.0f; // NormalisableRange skew factor (1.0 = linear)
};

struct KnobRangeSet
{
    // Keyed by APVTS parameter ID string.
    std::map<juce::String, KnobRange> ranges;

    KnobRangeSet()
    {
        initDefaults();
    }

    void initDefaults()
    {
        // ---- INPUT ----
        ranges["inputTrim"]           = { -20.0f,  20.0f, 1.0f };
        ranges["inputGain"]           = {   0.0f,  60.0f, 1.0f };
        ranges["outputVolume"]        = { -40.0f,   0.0f, 1.0f };

        // ---- GATE ----
        ranges["noiseGateThreshold"]  = { -80.0f, -20.0f, 1.0f };

        // ---- AMP ----
        ranges["masterVolume"]        = { -40.0f,   0.0f, 1.0f };

        // ---- PRE COMP ----
        ranges["preCompThresh"]       = { -60.0f,   0.0f, 1.0f };
        ranges["preCompRatio"]        = {   1.0f,  20.0f, 1.0f };
        ranges["preCompAttack"]       = {   1.0f, 200.0f, 1.0f };
        ranges["preCompRelease"]      = {  10.0f, 2000.0f, 1.0f };
        ranges["preCompMakeup"]       = {   0.0f,  20.0f, 1.0f };
        ranges["preCompBlend"]        = {   0.0f, 100.0f, 1.0f };

        // ---- POST COMP ----
        ranges["postCompThresh"]      = { -60.0f,   0.0f, 1.0f };
        ranges["postCompRatio"]       = {   1.0f,  20.0f, 1.0f };
        ranges["postCompAttack"]      = {   1.0f, 200.0f, 1.0f };
        ranges["postCompRelease"]     = {  10.0f, 2000.0f, 1.0f };
        ranges["postCompMakeup"]      = {   0.0f,  20.0f, 1.0f };
        ranges["postCompBlend"]       = {   0.0f, 100.0f, 1.0f };
    }
};
