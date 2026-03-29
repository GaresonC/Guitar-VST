#include "Tuner.h"
#include <cmath>
#include <numeric>

const char* Tuner::noteNames[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

Tuner::Tuner()
{
    buf.resize(kBufSize, 0.0f);
}

void Tuner::prepare(double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    std::fill(buf.begin(), buf.end(), 0.0f);
    writePos = 0;
    atomSignal.store(false);
    atomNote.store(-1);
}

void Tuner::process(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* data    = buffer.getReadPointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        buf[writePos++] = data[i];
        if (writePos >= kBufSize)
        {
            writePos = 0;
            runAnalysis();
        }
    }
}

void Tuner::runAnalysis()
{
    float freq = yinDetect();

    if (freq > 20.0f && freq < 2000.0f)
    {
        int   note  = freqToMidi(freq);
        float cents = freqToCents(freq, note);
        atomFreq.store(freq);
        atomNote.store(note);
        atomCents.store(cents);
        atomSignal.store(true);
    }
    else
    {
        atomSignal.store(false);
        atomNote.store(-1);
    }
}

// YIN pitch detection algorithm (de Cheveigné & Kawahara, 2002).
//
// Steps:
//   1. Difference function — autocorrelation-like sum of squared differences
//      for each lag τ from 1 to N/2.
//   2. Cumulative mean normalised difference (CMND) — divides each d(τ) by
//      the running mean of all d(1..τ), removing the downward bias at small τ.
//   3. Absolute threshold — finds the first τ where CMND < 0.15 and walks
//      forward to the local minimum (avoids octave errors). Falls back to the
//      global CMND minimum if no dip crosses the threshold (confidence gate: 0.5).
//   4. Parabolic interpolation — refines τ to sub-sample accuracy using the
//      three points around the minimum.
//
// Returns estimated frequency = sampleRate / τ_refined, or 0 if no pitch found.
float Tuner::yinDetect() const
{
    const int N    = kBufSize;
    const int half = N / 2;

    // Check RMS — skip if silence
    float rmsSum = 0.0f;
    for (int i = 0; i < N; ++i)
        rmsSum += buf[i] * buf[i];
    if (std::sqrt(rmsSum / N) < 0.001f) return 0.0f;

    // Step 1 + 2: difference function and CMND in one pass
    std::vector<float> d(half, 0.0f);
    for (int tau = 1; tau < half; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < half; ++j)
        {
            float diff = buf[j] - buf[j + tau];
            sum += diff * diff;
        }
        d[tau] = sum;
    }

    // Cumulative mean normalised difference
    std::vector<float> dp(half, 1.0f);
    dp[0] = 1.0f;
    float running = 0.0f;
    for (int tau = 1; tau < half; ++tau)
    {
        running += d[tau];
        dp[tau] = (running > 0.0f) ? d[tau] * tau / running : 1.0f;
    }

    // Step 3: find first tau below threshold
    const float threshold = 0.15f;
    int tauEst = -1;
    for (int tau = 2; tau < half; ++tau)
    {
        if (dp[tau] < threshold)
        {
            while (tau + 1 < half && dp[tau + 1] < dp[tau])
                ++tau;
            tauEst = tau;
            break;
        }
    }

    if (tauEst < 2)
    {
        // Fall back to global minimum
        tauEst = 2;
        for (int tau = 3; tau < half; ++tau)
            if (dp[tau] < dp[tauEst])
                tauEst = tau;

        if (dp[tauEst] > 0.5f) return 0.0f; // Low confidence
    }

    // Step 4: parabolic interpolation
    float refined = static_cast<float>(tauEst);
    if (tauEst > 1 && tauEst < half - 1)
    {
        float s0 = dp[tauEst - 1];
        float s1 = dp[tauEst];
        float s2 = dp[tauEst + 1];
        float denom = 2.0f * (s0 - 2.0f * s1 + s2);
        if (std::abs(denom) > 1e-6f)
            refined = tauEst + (s0 - s2) / denom;
    }

    return static_cast<float>(sampleRate) / refined;
}

int Tuner::freqToMidi(float freq)
{
    return static_cast<int>(std::round(69.0f + 12.0f * std::log2(freq / 440.0f)));
}

float Tuner::freqToCents(float freq, int midiNote)
{
    const float exact = 69.0f + 12.0f * std::log2(freq / 440.0f);
    return (exact - static_cast<float>(midiNote)) * 100.0f;
}

TunerResult Tuner::getResult() const
{
    TunerResult r;
    r.frequency = atomFreq.load();
    r.midiNote  = atomNote.load();
    r.cents     = atomCents.load();
    r.hasSignal = atomSignal.load();
    return r;
}
