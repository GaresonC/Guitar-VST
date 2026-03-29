#pragma once
#include <JuceHeader.h>

// 8-band parametric EQ, placed post-IR in the signal chain.
// Band 0: low shelf, bands 1–6: peak filters (Q=1.4, ~0.7 octave), band 7: high shelf.
// Default centre frequencies: 80, 250, 500, 1k, 2k, 4k, 8k, 16k Hz (user-adjustable).
// Processes per-sample through all 8 filters in cascade per channel.
class EQProcessor
{
public:
    static constexpr int  kNumBands = 8;
    static const float    kBandFreqs[kNumBands];

    EQProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);
    void update(const float gains[kNumBands], const float freqs[kNumBands]);

private:
    static constexpr int kMaxChannels = 2;

    double sampleRate = 44100.0;
    float  gainDb[kNumBands] = {};
    float  freqHz[kNumBands] = {};

    juce::dsp::IIR::Filter<float> filters[kNumBands][kMaxChannels];

    void updateFilters();
};
