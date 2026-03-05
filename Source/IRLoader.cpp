#include "IRLoader.h"

IRLoader::IRLoader() {}

void IRLoader::prepare(double sampleRate, int samplesPerBlock, int numChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32)juce::jmax(1, samplesPerBlock);
    spec.numChannels      = (juce::uint32)juce::jmax(1, numChannels);
    convolution.prepare(spec);
}

void IRLoader::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled || !hasIR) return;

    juce::dsp::AudioBlock<float>            block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    convolution.process(ctx);
}

bool IRLoader::loadIR(const juce::File& file)
{
    if (!file.existsAsFile()) return false;

    convolution.loadImpulseResponse(
        file,
        juce::dsp::Convolution::Stereo::no,
        juce::dsp::Convolution::Trim::yes,
        0,
        juce::dsp::Convolution::Normalise::yes);

    fileName = file.getFileName();
    filePath = file.getFullPathName();
    hasIR    = true;
    return true;
}

void IRLoader::setEnabled(bool e)
{
    enabled = e;
}
