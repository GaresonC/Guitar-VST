#pragma once
#include <JuceHeader.h>

// Cabinet impulse-response loader — wraps juce::dsp::Convolution.
// Two load paths: from a WAV file on disk, or from in-memory BinaryData (bundled IRs).
// Convolution is mono, trimmed, and normalised.
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
