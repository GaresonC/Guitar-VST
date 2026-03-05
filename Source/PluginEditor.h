#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EQProcessor.h"

//==============================================================================
// Tuner display — draws note name, cents bar and cents value.
// Runs its own 15 Hz timer to poll atomic results from the processor.
class TunerDisplay : public juce::Component,
                     private juce::Timer
{
public:
    explicit TunerDisplay(GuitarAmpAudioProcessor& p);
    ~TunerDisplay() override;

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    GuitarAmpAudioProcessor& processor;
    TunerResult current;
};

//==============================================================================
class GuitarAmpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit GuitarAmpAudioProcessorEditor(GuitarAmpAudioProcessor&);
    ~GuitarAmpAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    GuitarAmpAudioProcessor& audioProcessor;

    // Tuner section
    TunerDisplay tunerDisplay;
    juce::TextButton tunerToggle { "TUNER" };

    // Amp knobs
    juce::Slider gainSlider, bassSlider, midSlider,
                 trebleSlider, presenceSlider, masterSlider;
    juce::Label  gainLabel, bassLabel, midLabel,
                 trebleLabel, presenceLabel, masterLabel;

    // Noise gate section
    juce::TextButton gateEnableBtn   { "GATE" };
    juce::Slider     gateThreshSlider;
    juce::Label      gateThreshLabel;

    // Pitch shifter section
    juce::TextButton pitchEnableBtn  { "PITCH" };
    juce::Slider     pitchSemiSlider;
    juce::Label      pitchSemiLabel;

    // IR loader section
    juce::ComboBox   irPresetBox;
    juce::TextButton loadIRBtn  { "Browse..." };
    juce::TextButton irOnBtn    { "IR ON" };
    juce::Label      irFileLabel;

    // Pre-amp stage (EQ + compressor)
    juce::Slider preEqLowSlider, preEqMidSlider, preEqHighSlider,
                 preCompThreshSlider, preCompRatioSlider;
    juce::Label  preEqLowLabel, preEqMidLabel, preEqHighLabel,
                 preCompThreshLabel, preCompRatioLabel;

    // Post-amp stage (EQ + compressor)
    juce::Slider postEqLowSlider, postEqMidSlider, postEqHighSlider,
                 postCompThreshSlider, postCompRatioSlider;
    juce::Label  postEqLowLabel, postEqMidLabel, postEqHighLabel,
                 postCompThreshLabel, postCompRatioLabel;

    // Post-IR 8-band EQ
    juce::Slider eqSliders[EQProcessor::kNumBands];
    juce::Label  eqLabels [EQProcessor::kNumBands];

    // APVTS attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> gainAtt, bassAtt, midAtt,
                                trebleAtt, presenceAtt, masterAtt;
    std::unique_ptr<SliderAtt> gateThreshAtt;
    std::unique_ptr<ButtonAtt> irEnabledAtt, tunerEnabledAtt, gateEnabledAtt;
    std::unique_ptr<ButtonAtt> pitchEnabledAtt;
    std::unique_ptr<SliderAtt> pitchSemiAtt;
    std::unique_ptr<SliderAtt> eqAtts[EQProcessor::kNumBands];

    std::unique_ptr<SliderAtt> preEqLowAtt, preEqMidAtt, preEqHighAtt,
                                preCompThreshAtt, preCompRatioAtt;
    std::unique_ptr<SliderAtt> postEqLowAtt, postEqMidAtt, postEqHighAtt,
                                postCompThreshAtt, postCompRatioAtt;

    // File chooser must outlive its callback
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Helpers
    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setupLargeKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void styleButton(juce::TextButton& b, bool isToggle = false);
    void loadIRFile();
    void syncIRPresetBox();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessorEditor)
};
