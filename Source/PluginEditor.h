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
    juce::TextButton tunerToggle { "MUTE" };

    // Amp controls
    juce::Slider     gainSlider, masterSlider;
    juce::Label      gainLabel, masterLabel;
    juce::TextButton loadModelBtn { "Browse..." };
    juce::Label      modelFileLabel;

    // Noise gate section
    juce::TextButton gateEnableBtn   { "GATE" };
    juce::Slider     gateThreshSlider;
    juce::Label      gateThreshLabel;

    // Pitch shifter section
    juce::TextButton pitchEnableBtn  { "PITCH" };
    juce::Slider     pitchSemiSlider;
    juce::Label      pitchSemiLabel;

    // Preset manager
    juce::ComboBox   presetBox;
    juce::TextButton savePresetBtn   { "SAVE" };
    juce::TextButton deletePresetBtn { "DEL"  };

    // IR loader section
    juce::ComboBox   irPresetBox;
    juce::TextButton loadIRBtn  { "Browse..." };
    juce::TextButton irOnBtn    { "IR ON" };
    juce::Label      irFileLabel;

    // Pre-amp EQ
    juce::Slider preEqLowSlider, preEqMidSlider, preEqHighSlider;
    juce::Label  preEqLowLabel, preEqMidLabel, preEqHighLabel;
    juce::Slider preEqLowFreqSlider, preEqMidFreqSlider, preEqHighFreqSlider;
    juce::Label  preEqLowFreqLabel, preEqMidFreqLabel, preEqHighFreqLabel;

    // Pre-amp compressor
    juce::Slider preCompThreshSlider, preCompRatioSlider;
    juce::Slider preCompAttackSlider, preCompReleaseSlider, preCompMakeupSlider;
    juce::Label  preCompThreshLabel, preCompRatioLabel;
    juce::Label  preCompAttackLabel, preCompReleaseLabel, preCompMakeupLabel;

    // Post-amp EQ
    juce::Slider postEqLowSlider, postEqMidSlider, postEqHighSlider;
    juce::Label  postEqLowLabel, postEqMidLabel, postEqHighLabel;
    juce::Slider postEqLowFreqSlider, postEqMidFreqSlider, postEqHighFreqSlider;
    juce::Label  postEqLowFreqLabel, postEqMidFreqLabel, postEqHighFreqLabel;

    // Post-amp compressor
    juce::Slider postCompThreshSlider, postCompRatioSlider;
    juce::Slider postCompAttackSlider, postCompReleaseSlider, postCompMakeupSlider;
    juce::Label  postCompThreshLabel, postCompRatioLabel;
    juce::Label  postCompAttackLabel, postCompReleaseLabel, postCompMakeupLabel;

    // Post-IR 8-band EQ
    juce::Slider eqSliders[EQProcessor::kNumBands];
    juce::Label  eqLabels [EQProcessor::kNumBands];

    // APVTS attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAtt> gainAtt, masterAtt;
    std::unique_ptr<SliderAtt> gateThreshAtt;
    std::unique_ptr<ButtonAtt> irEnabledAtt, muteEnabledAtt, gateEnabledAtt;
    std::unique_ptr<ButtonAtt> pitchEnabledAtt;
    std::unique_ptr<SliderAtt> pitchSemiAtt;
    std::unique_ptr<SliderAtt> eqAtts[EQProcessor::kNumBands];

    std::unique_ptr<SliderAtt> preEqLowAtt, preEqMidAtt, preEqHighAtt;
    std::unique_ptr<SliderAtt> preEqLowFreqAtt, preEqMidFreqAtt, preEqHighFreqAtt;
    std::unique_ptr<SliderAtt> preCompThreshAtt, preCompRatioAtt;
    std::unique_ptr<SliderAtt> preCompAttackAtt, preCompReleaseAtt, preCompMakeupAtt;
    std::unique_ptr<SliderAtt> postEqLowAtt, postEqMidAtt, postEqHighAtt;
    std::unique_ptr<SliderAtt> postEqLowFreqAtt, postEqMidFreqAtt, postEqHighFreqAtt;
    std::unique_ptr<SliderAtt> postCompThreshAtt, postCompRatioAtt;
    std::unique_ptr<SliderAtt> postCompAttackAtt, postCompReleaseAtt, postCompMakeupAtt;

    // File choosers must outlive their callbacks
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> modelFileChooser;

    // Helpers
    void refreshPresetList();
    void saveCurrentPreset();
    void deleteCurrentPreset();
    juce::File getPresetDirectory();

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setupFreqSlider(juce::Slider& s, juce::Label& l);
    void setupCompKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setupLargeKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void styleButton(juce::TextButton& b, bool isToggle = false);
    void loadIRFile();
    void loadModelFile();
    void syncIRPresetBox();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessorEditor)
};
