#pragma once
#include <JuceHeader.h>

// Phase-vocoder pitch shifter using time-stretch + resample approach.
// Processes each channel independently.
// Latency = kFFTSize samples (~5.8 ms @ 44.1 kHz).
class PitchShifter
{
public:
    PitchShifter() = default;
    void prepare (double sampleRate, int maxBlockSize);
    void process (juce::AudioBuffer<float>& buffer, int semitones);
    void reset();

    int getLatencyInSamples() const noexcept { return kFFTSize; }

private:
    // 256-point FFT, 8x overlap (hop = 32).  Gives ~5.8 ms latency @ 44.1 kHz.
    static constexpr int kFFTOrder = 8;
    static constexpr int kFFTSize  = 1 << kFFTOrder;   // 256
    static constexpr int kHopSize  = kFFTSize / 8;     // 32
    static constexpr int kNumBins  = kFFTSize / 2 + 1; // 129
    static constexpr int kOutBufSize = kFFTSize * 4;   // 1024

    struct Channel
    {
        float  inBuf   [kFFTSize]   = {};  // circular input ring buffer
        float  outBuf  [kOutBufSize]= {};  // overlap-add output accumulator
        float  lastPhs [kNumBins]   = {};  // analysis phase from previous hop
        float  synthPhs[kNumBins]   = {};  // running synthesis phase accumulator
        int    inWritePos  = 0;
        int    outWritePos = 0;            // integer write position into outBuf
        double outReadFrac = 0.0;          // fractional read position into outBuf
        int    hopCount    = 0;
    };

    Channel channels[2];

    juce::dsp::FFT fft { kFFTOrder };
    float window [kFFTSize]     = {};
    float fftIn  [kFFTSize * 2] = {};   // separate input copy to avoid in-place FFT UB
    float fftWork[kFFTSize * 2] = {};   // interleaved complex work buffer

    int prevSemitones = 0;

    void processHop (Channel& ch, int synthHop);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifter)
};
