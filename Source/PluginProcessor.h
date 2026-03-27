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

class GuitarAmpAudioProcessor : public juce::AudioProcessor
{
public:
    GuitarAmpAudioProcessor();
    ~GuitarAmpAudioProcessor() override;

    //==========================================================================
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
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState apvts;
    KnobRangeSet                 knobRanges;
    SectionColourSet             sectionColours;
    juce::ValueTree              sectionImagesTree { "SectionImages" };

    NeuralAmpProcessor           neuralAmp;
    StageProcessor               preAmpStage  { 200.f, 700.f,  4500.f };
    StageProcessor               postAmpStage { 150.f, 1000.f, 5000.f };
    Tuner                        tuner;
    IRLoader                     irLoader;
    EQProcessor                  eqProcessor;
    juce::dsp::NoiseGate<float>  noiseGate;
    std::unique_ptr<Cloudseed::ReverbController> reverb;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessor)
};
