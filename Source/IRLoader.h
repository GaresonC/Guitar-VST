#pragma once
#include <JuceHeader.h>

class IRLoader
{
public:
    IRLoader();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void process(juce::AudioBuffer<float>& buffer);

    bool loadIR(const juce::File& file);
    void setEnabled(bool enabled);

    bool          isEnabled()     const { return enabled; }
    bool          hasLoadedIR()   const { return hasIR;   }
    juce::String  getFileName()   const { return fileName; }

private:
    juce::dsp::Convolution convolution;
    bool         enabled  = false;
    bool         hasIR    = false;
    juce::String fileName;
};
