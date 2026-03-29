#pragma once
#include <JuceHeader.h>
#include "NeuralAmpProcessor.h"
#include "StageProcessor.h"
#include "Tuner.h"
#include "IRLoader.h"
#include "EQProcessor.h"
#include "KnobRangeSet.h"
#include "SectionColourSet.h"
#include "CloudSeed/DSP/ReverbController.h"

/** Main JUCE AudioProcessor for MF AMP.
 *
 *  Owns all DSP modules and routes audio through the signal chain in processBlock():
 *    Input Trim → Tuner → Noise Gate → Pre-Amp Stage → Neural Amp →
 *    Post-Amp Stage → IR Loader → (stereo copy) → Reverb → MF EQ → Output Volume → Mute
 *
 *  All user-facing parameters are registered in the APVTS (see createParameterLayout()).
 *  State is serialised to XML in getStateInformation(), including the IR path, neural
 *  model path, knob ranges, section images, and section colours — everything beyond the
 *  raw APVTS parameter values.
 */
class GuitarAmpAudioProcessor : public juce::AudioProcessor
{
public:
    GuitarAmpAudioProcessor();
    ~GuitarAmpAudioProcessor() override;

    //==========================================================================
    // JUCE AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    //==========================================================================
    int  getNumPrograms()                                       override { return 1; }
    int  getCurrentProgram()                                    override { return 0; }
    void setCurrentProgram(int)                                 override {}
    const juce::String getProgramName(int)                      override { return {}; }
    void changeProgramName(int, const juce::String&)            override {}

    //==========================================================================
    // State persistence — serialises APVTS plus IR path, model path, knob ranges,
    // section images, and section colours.
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    // Public state — accessed directly by PluginEditor
    juce::AudioProcessorValueTreeState apvts;        ///< All automatable parameters
    KnobRangeSet                 knobRanges;          ///< User-editable display ranges per knob
    SectionColourSet             sectionColours;      ///< User-editable border colours per UI section
    juce::ValueTree              sectionImagesTree { "SectionImages" }; ///< Per-section background images (base64 PNG)

    // DSP modules (signal chain order)
    NeuralAmpProcessor           neuralAmp;
    StageProcessor               preAmpStage  { 200.f, 700.f,  4500.f }; ///< EQ+comp before distortion
    StageProcessor               postAmpStage { 150.f, 1000.f, 5000.f }; ///< EQ+comp after distortion
    Tuner                        tuner;
    IRLoader                     irLoader;
    EQProcessor                  eqProcessor;   ///< 8-band post-IR parametric EQ
    juce::dsp::NoiseGate<float>  noiseGate;
    std::unique_ptr<Cloudseed::ReverbController> reverb; ///< Created/reset in prepareToPlay

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessor)
};
