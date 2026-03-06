#pragma once
#include <JuceHeader.h>

// Phase-vocoder pitch shifter.
// Processes each channel independently (no stereo tricks).
// Latency = kFFTSize samples (~5.8 ms @ 44.1 kHz).  The DAW should be told
// via setLatencySamples() in prepareToPlay if DAW track-alignment matters.
class PitchShifter
{
public:
    PitchShifter() = default;
    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& buffer, int semitones);
    void reset();

    int getLatencyInSamples() const noexcept { return kFFTSize; }

private:
    // 256-point FFT, 4x overlap (hop = 64).  Gives ~5.8 ms latency @ 44.1 kHz.
    static constexpr int kFFTOrder = 8;
    static constexpr int kFFTSize  = 1 << kFFTOrder;   // 256
    static constexpr int kHopSize  = kFFTSize / 4;     // 64
    static constexpr int kNumBins  = kFFTSize / 2 + 1; // 129

    struct Channel
    {
        float inBuf   [kFFTSize]     = {};  // circular input ring buffer
        float outBuf  [kFFTSize * 2] = {};  // overlap-add output accumulator
        float lastPhs [kNumBins]     = {};  // analysis phase from previous hop
        float synthPhs[kNumBins]     = {};  // running synthesis phase accumulator
        int   inWritePos = 0;
        int   outReadPos = 0;
        int   hopCount   = 0;
    };

    Channel channels[2];

    juce::dsp::FFT fft { kFFTOrder };
    float window [kFFTSize]     = {};
    float fftWork[kFFTSize * 2] = {};   // interleaved complex work buffer

    int prevSemitones = 0;

    void processHop (Channel& ch, float pitchRatio);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifter)
};
