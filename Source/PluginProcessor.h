#pragma once
#include <JuceHeader.h>
#include "AmpProcessor.h"
#include "Tuner.h"
#include "IRLoader.h"

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
    Tuner                        tuner;
    IRLoader                     irLoader;
    juce::dsp::NoiseGate<float>  noiseGate;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarAmpAudioProcessor)
};
