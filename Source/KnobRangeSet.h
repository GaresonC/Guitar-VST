#pragma once
#include <JuceHeader.h>
#include <map>

//==============================================================================
// Stores per-parameter display ranges that the user can edit via the
// "Knob Ranges..." settings dialog. Ranges are saved alongside the APVTS
// state in presets.
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
        ranges["inputGain"]           = { -20.0f,  10.0f, 1.0f };
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

        // ---- MF EQ ----
        ranges["eq1Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq2Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq3Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq4Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq5Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq6Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq7Gain"]             = { -15.0f,  15.0f, 1.0f };
        ranges["eq8Gain"]             = { -15.0f,  15.0f, 1.0f };

        // ---- REVERB ----
        ranges["reverbMix"]           = {   0.0f, 100.0f, 1.0f };
        ranges["reverbDecay"]         = {   0.0f, 100.0f, 1.0f };
        ranges["reverbSize"]          = {   0.0f, 100.0f, 1.0f };

        // ---- MF EQ FREQ ----
        ranges["eq1Freq"]             = {  20.0f,   500.0f, 0.3f };
        ranges["eq2Freq"]             = { 100.0f,   800.0f, 0.3f };
        ranges["eq3Freq"]             = { 200.0f,  1500.0f, 0.3f };
        ranges["eq4Freq"]             = { 400.0f,  3000.0f, 0.3f };
        ranges["eq5Freq"]             = { 800.0f,  6000.0f, 0.3f };
        ranges["eq6Freq"]             = { 1500.0f, 10000.0f, 0.3f };
        ranges["eq7Freq"]             = { 3000.0f, 16000.0f, 0.3f };
        ranges["eq8Freq"]             = { 6000.0f, 20000.0f, 0.3f };
    }
};
