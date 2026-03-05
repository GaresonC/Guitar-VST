#pragma once
#include <JuceHeader.h>

// A signal-shaping stage: 3-band EQ (low shelf, mid peak, high shelf) + soft-knee compressor.
// Used for both pre-amp input shaping and post-amp output shaping.
class StageProcessor
{
public:
    // lowFreqHz / midFreqHz / highFreqHz: EQ band centre frequencies
    // attackMs / releaseMs: compressor envelope timing
    StageProcessor(float lowFreqHz, float midFreqHz, float highFreqHz,
                   float attackMs, float releaseMs);

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    // lowDb, midDb, highDb: EQ gain in dB
    // compThreshDb: threshold in dBFS (0 dB = effectively off)
    // compRatio: linear compression ratio (1 = no compression)
    void update(float lowDb, float midDb, float highDb,
                float compThreshDb, float compRatio);

private:
    static constexpr int kMaxChannels = 2;

    const float kLowFreq, kMidFreq, kHighFreq;
    const float kAttackMs, kReleaseMs;

    double sampleRate  = 44100.0;

    float lowGainDb    = 0.0f;
    float midGainDb    = 0.0f;
    float highGainDb   = 0.0f;
    float compThreshDb = 0.0f;   // 0 dB default = off (signal never reaches 0 dBFS)
    float compRatio    = 3.0f;

    juce::dsp::IIR::Filter<float> lowFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> midFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> highFilter[kMaxChannels];

    float env[kMaxChannels] = {};
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    void  updateFilters();
    float applyComp(float x, float& e) const;
};
