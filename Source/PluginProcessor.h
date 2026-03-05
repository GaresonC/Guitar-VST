#pragma once
#include <JuceHeader.h>
#include "AmpProcessor.h"
#include "StageProcessor.h"
#include "Tuner.h"
#include "IRLoader.h"
#include "EQProcessor.h"
#include "PitchShifter.h"

class GuitarAmpAudioProcessor : public juce::AudioProcessor
{
public:
    GuitarAmpAudioProcessor();
    ~GuitarAmpAudioProcessor() override;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
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

    AmpProcessor                 ampProcessor;
    StageProcessor               preAmpStage  { 200.f, 700.f,  4500.f, 8.f,  50.f };
    StageProcessor               postAmpStage { 150.f, 1000.f, 5000.f, 2.f,  80.f };
    Tuner                        tuner;
    IRLoader                     irLoader;
    EQProcessor                  eqProcessor;
    juce::dsp::NoiseGate<float>  noiseGate;
    PitchShifter                 pitchShifter;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessor)
};
