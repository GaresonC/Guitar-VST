#pragma once
#include <JuceHeader.h>

// A signal-shaping stage: 3-band EQ (low shelf, mid peak, high shelf) + soft-knee compressor.
// Used for both pre-amp input shaping and post-amp output shaping.
class StageProcessor
{
public:
    // lowFreqHz / midFreqHz / highFreqHz: default EQ band centre frequencies
    StageProcessor(float lowFreqHz, float midFreqHz, float highFreqHz);

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    // lowDb, midDb, highDb: EQ gain in dB
    // lowHz, midHz, highHz: EQ band centre frequencies in Hz
    // compThreshDb: threshold in dBFS
    // compRatio: linear compression ratio (1 = no compression)
    // compAttackMs / compReleaseMs: envelope timing in milliseconds
    // compMakeupDb: makeup gain applied after compression
    void update(float lowDb, float midDb, float highDb,
                float lowHz, float midHz, float highHz,
                float compThreshDb, float compRatio,
                float compAttackMs, float compReleaseMs,
                float compMakeupDb);

private:
    static constexpr int kMaxChannels = 2;

    float kLowFreq, kMidFreq, kHighFreq;
    float attackMs  = 10.0f;
    float releaseMs = 100.0f;

    double sampleRate  = 44100.0;

    float lowGainDb    = 0.0f;
    float midGainDb    = 0.0f;
    float highGainDb   = 0.0f;
    float compThreshDb   = 0.0f;   // 0 dB default = off (signal never reaches 0 dBFS)
    float compRatio      = 3.0f;
    float compMakeupGain = 1.0f;   // linear makeup gain (cached from dB)

    juce::dsp::IIR::Filter<float> lowFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> midFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> highFilter[kMaxChannels];

    float env[kMaxChannels] = {};
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    void  updateFilters();
    float applyComp(float x, float& e) const;
};
