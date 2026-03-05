#pragma once
#include <JuceHeader.h>

class EQProcessor
{
public:
    static constexpr int  kNumBands = 8;
    static const float    kBandFreqs[kNumBands];

    EQProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    void update(const float gains[kNumBands]);

private:
    static constexpr int kMaxChannels = 2;

    double sampleRate = 44100.0;
    float  gainDb[kNumBands] = {};

    juce::dsp::IIR::Filter<float> filters[kNumBands][kMaxChannels];

    void updateFilters();
};
