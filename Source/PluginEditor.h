#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EQProcessor.h"
#include "KnobRangesDialog.h"

//==============================================================================
// Custom LookAndFeel: flat arc-style rotary knobs (Serum/Vital style).
// Reads rotarySliderFillColourId from each slider, so orange/green per-section
// colours are inherited automatically from the existing setColour() calls.
class GuitarAmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPos,
                          float startAngle, float endAngle,
                          juce::Slider& slider) override;
};

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

    // Called from processor/dialog when knob ranges change
    void rebuildAllAttachments();

    // Called from processor when DAW state is restored
    void loadSectionImagesFromTree();

private:
    GuitarAmpLookAndFeel ampLookAndFeel; // must be declared before all Slider members

    GuitarAmpAudioProcessor& audioProcessor;

    // Tuner section
    TunerDisplay tunerDisplay;
    juce::TextButton tunerToggle { "MUTE" };
    juce::TextButton settingsBtn { "SETTINGS" };

    // Amp controls
    juce::Slider     gainSlider, masterSlider;
    juce::Label      gainLabel, masterLabel;
    juce::TextButton loadModelBtn { "Browse..." };
    juce::Label      modelFileLabel;

    // Noise gate section
    juce::TextButton gateEnableBtn   { "GATE" };
    juce::Slider     gateThreshSlider;
    juce::Label      gateThreshLabel;

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
    juce::Slider preCompAttackSlider, preCompReleaseSlider, preCompMakeupSlider, preCompBlendSlider;
    juce::Label  preCompThreshLabel, preCompRatioLabel;
    juce::Label  preCompAttackLabel, preCompReleaseLabel, preCompMakeupLabel, preCompBlendLabel;

    // Post-amp EQ
    juce::Slider postEqLowSlider, postEqMidSlider, postEqHighSlider;
    juce::Label  postEqLowLabel, postEqMidLabel, postEqHighLabel;
    juce::Slider postEqLowFreqSlider, postEqMidFreqSlider, postEqHighFreqSlider;
    juce::Label  postEqLowFreqLabel, postEqMidFreqLabel, postEqHighFreqLabel;

    // Post-amp compressor
    juce::Slider postCompThreshSlider, postCompRatioSlider;
    juce::Slider postCompAttackSlider, postCompReleaseSlider, postCompMakeupSlider, postCompBlendSlider;
    juce::Label  postCompThreshLabel, postCompRatioLabel;
    juce::Label  postCompAttackLabel, postCompReleaseLabel, postCompMakeupLabel, postCompBlendLabel;

    // Amp input trim
    juce::Slider inputTrimSlider;
    juce::Label  inputTrimLabel;

    // Post-IR 8-band EQ
    juce::Slider eqSliders[EQProcessor::kNumBands];
    juce::Label  eqLabels [EQProcessor::kNumBands];

    // Output volume section
    juce::Slider outputVolSlider;
    juce::Label  outputVolLabel;

    // APVTS attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAtt> gainAtt, masterAtt;
    std::unique_ptr<SliderAtt> gateThreshAtt;
    std::unique_ptr<ButtonAtt> irEnabledAtt, muteEnabledAtt, gateEnabledAtt;
    std::unique_ptr<SliderAtt> eqAtts[EQProcessor::kNumBands];
    std::unique_ptr<SliderAtt> outputVolAtt;

    std::unique_ptr<SliderAtt> preEqLowAtt, preEqMidAtt, preEqHighAtt;
    std::unique_ptr<SliderAtt> preEqLowFreqAtt, preEqMidFreqAtt, preEqHighFreqAtt;
    std::unique_ptr<SliderAtt> preCompThreshAtt, preCompRatioAtt;
    std::unique_ptr<SliderAtt> preCompAttackAtt, preCompReleaseAtt, preCompMakeupAtt, preCompBlendAtt;
    std::unique_ptr<SliderAtt> postEqLowAtt, postEqMidAtt, postEqHighAtt;
    std::unique_ptr<SliderAtt> postEqLowFreqAtt, postEqMidFreqAtt, postEqHighFreqAtt;
    std::unique_ptr<SliderAtt> postCompThreshAtt, postCompRatioAtt;
    std::unique_ptr<SliderAtt> postCompAttackAtt, postCompReleaseAtt, postCompMakeupAtt, postCompBlendAtt;
    std::unique_ptr<SliderAtt> inputTrimAtt;

    // Section background images
    enum SectionId {
        kInput=0, kGate, kPreEq, kPreComp, kAmp, kCabinet,
        kPostEq, kPostComp, kMfEq, kOutput,
        kOverallBg,
        kNumSections
    };

    struct SectionImageData {
        juce::Image image;
        float offsetX = 0.f;
        float offsetY = 0.f;
        float scale   = 1.f;
    };

    std::array<SectionImageData, kNumSections> sectionImages;

    // File choosers must outlive their callbacks
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> modelFileChooser;
    std::unique_ptr<juce::FileChooser> imageFileChooser;

    // Helpers
    void refreshPresetList();
    void saveCurrentPreset();
    void deleteCurrentPreset();
    juce::File getPresetDirectory();

    juce::Rectangle<int> getSectionRect(int sectionId) const;
    void showImageManagementMenu();
    void loadSectionImage(int sectionId);
    void centerImageInSection(int sectionId);
    void saveSectionImagesToTree();

    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setupFreqSlider(juce::Slider& s, juce::Label& l);
    void setupCompKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void setupLargeKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void styleButton(juce::TextButton& b, bool isToggle = false);
    void loadIRFile();
    void loadModelFile();
    void showSettingsMenu();
    void syncIRPresetBox();
    void applyKnobRange(juce::Slider& s, const juce::String& paramId);
    void applyAllKnobRanges();


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessorEditor)
};
