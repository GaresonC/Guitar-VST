#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

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

    // Channel selection
    juce::TextButton cleanBtn  { "CLEAN"  };
    juce::TextButton crunchBtn { "CRUNCH" };
    juce::TextButton leadBtn   { "LEAD"   };

    // Amp knobs
    juce::Slider gainSlider, bassSlider, midSlider,
                 trebleSlider, presenceSlider, masterSlider;
    juce::Label  gainLabel, bassLabel, midLabel,
                 trebleLabel, presenceLabel, masterLabel;

    // IR loader section
    juce::TextButton loadIRBtn  { "Load IR..." };
    juce::TextButton irOnBtn    { "IR ON" };
    juce::Label      irFileLabel;

    // APVTS attachments
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAtt> gainAtt, bassAtt, midAtt,
                                trebleAtt, presenceAtt, masterAtt;
    std::unique_ptr<ButtonAtt> irEnabledAtt, tunerEnabledAtt;

    // File chooser must outlive its callback
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Helpers
    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
    void styleButton(juce::TextButton& b, bool isToggle = false);
    void loadIRFile();
    void syncChannelButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessorEditor)
};
