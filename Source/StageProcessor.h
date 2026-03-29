#pragma once
#include <JuceHeader.h>

// A signal-shaping stage: 3-band EQ (low shelf, mid peak, high shelf) + soft-knee compressor.
// Used for both pre-amp input shaping and post-amp output shaping.
class StageProcessor
{
public:
    // lowFreqHz / midFreqHz / highFreqHz: default EQ band centre frequencies
    StageProcessor(float lowFreqHz, float midFreqHz, float highFreqHz);

    /** Initialise all filters and compute attack/release envelope coefficients.
     *  Must be called before process() whenever the sample rate changes. */
    void prepare(double sampleRate, int samplesPerBlock);

    /** Process buffer in-place: 3-band EQ followed by parallel-blended soft-knee
     *  compression. Applies to all channels up to kMaxChannels. */
    void process(juce::AudioBuffer<float>& buffer);

    // lowDb, midDb, highDb: EQ gain in dB
    // lowHz, midHz, highHz: EQ band centre frequencies in Hz
    // compThreshDb: threshold in dBFS
    // compRatio: linear compression ratio (1 = no compression)
    // compAttackMs / compReleaseMs: envelope timing in milliseconds
    // compMakeupDb: makeup gain applied after compression
    // blendPct: 0–100 % parallel compression blend (0 = dry only, 100 = wet only).
    // EQ is always applied; blend only affects the compressor path.
    void update(float lowDb, float midDb, float highDb,
                float lowHz, float midHz, float highHz,
                float compThreshDb, float compRatio,
                float compAttackMs, float compReleaseMs,
                float compMakeupDb, float blendPct);

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
    float compBlend      = 1.0f;   // 0 = fully dry (post-EQ), 1 = fully compressed

    juce::dsp::IIR::Filter<float> lowFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> midFilter [kMaxChannels];
    juce::dsp::IIR::Filter<float> highFilter[kMaxChannels];

    float env[kMaxChannels] = {};
    float attackCoeff  = 0.0f;
    float releaseCoeff = 0.0f;

    void  updateFilters();
    float applyComp(float x, float& e) const;
};
