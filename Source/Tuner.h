#pragma once
#include <JuceHeader.h>
#include <vector>

struct TunerResult
{
    float frequency = 0.0f;
    int   midiNote  = -1;
    float cents     = 0.0f;
    bool  hasSignal = false;
};

// YIN pitch detector operating on channel 0.
// Fills a 2048-sample ring buffer; runs detection each time the buffer wraps.
// Results are stored in atomics so the UI can poll at 15 Hz without locking.
// Valid detection range: 20 Hz – 2 kHz.
class Tuner
{
public:
    Tuner();

    void prepare(double sampleRate, int samplesPerBlock);

    // Call from audio thread — processes channel 0 only
    void process(const juce::AudioBuffer<float>& buffer);

    // Safe to call from any thread
    TunerResult getResult() const;

    static const char* noteNames[12];

private:
    static constexpr int kBufSize = 2048;

    double sampleRate = 44100.0;

    std::vector<float> buf;
    int writePos = 0;

    std::atomic<float> atomFreq     { 0.0f };
    std::atomic<int>   atomNote     { -1 };
    std::atomic<float> atomCents    { 0.0f };
    std::atomic<bool>  atomSignal   { false };

    void runAnalysis();
    float yinDetect() const;

    static int   freqToMidi(float freq);
    static float freqToCents(float freq, int midiNote);
};
