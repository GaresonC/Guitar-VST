#pragma once
#include <JuceHeader.h>

class IRLoader
{
public:
    IRLoader();

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void process(juce::AudioBuffer<float>& buffer);

    bool loadIR(const juce::File& file);
    bool loadIR(const void* data, int sizeBytes, const juce::String& displayName);
    void setEnabled(bool enabled);

    bool          isEnabled()     const { return enabled;  }
    bool          hasLoadedIR()   const { return hasIR;    }
    juce::String  getFileName()   const { return fileName; }
    juce::String  getFilePath()   const { return filePath; }

private:
    juce::dsp::Convolution convolution;
    bool         enabled  = false;
    bool         hasIR    = false;
    juce::String fileName;
    juce::String filePath;
};
