#include "PitchShifter.h"

void PitchShifter::prepare (double /*sampleRate*/, int /*maxBlockSize*/)
{
    // Hann window
    for (int i = 0; i < kFFTSize; ++i)
        window[i] = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi
                                              * (float)i / (float)kFFTSize));
    reset();
}

void PitchShifter::reset()
{
    for (auto& ch : channels)
    {
        std::fill (std::begin (ch.inBuf),    std::end (ch.inBuf),    0.0f);
        std::fill (std::begin (ch.outBuf),   std::end (ch.outBuf),   0.0f);
        std::fill (std::begin (ch.lastPhs),  std::end (ch.lastPhs),  0.0f);
        std::fill (std::begin (ch.synthPhs), std::end (ch.synthPhs), 0.0f);
        ch.inWritePos = 0;
        ch.outReadPos = 0;
        ch.hopCount   = 0;
    }
}

// -------------------------------------------------------------------------
void PitchShifter::processHop (Channel& ch, float pitchRatio)
{
    using Complex = juce::dsp::Complex<float>;
    const float twoPi = juce::MathConstants<float>::twoPi;

    // ---- Analysis: window + forward FFT ------------------------------------
    for (int i = 0; i < kFFTSize; ++i)
    {
        // Read from circular input buffer, oldest sample first
        int idx = (ch.inWritePos - kFFTSize + i + kFFTSize) % kFFTSize;
        fftWork[2 * i]     = ch.inBuf[idx] * window[i];
        fftWork[2 * i + 1] = 0.0f;
    }

    std::copy (fftWork, fftWork + kFFTSize * 2, fftIn);
    fft.perform (reinterpret_cast<Complex*> (fftIn),
                 reinterpret_cast<Complex*> (fftWork), false);

    // ---- Phase vocoder: true-frequency estimation + bin remapping ----------
    float outMag   [kNumBins] = {};
    float outPhsInc[kNumBins] = {};  // magnitude-weighted phase increment, applied once per dest bin

    for (int k = 0; k < kNumBins; ++k)
    {
        const float re  = fftWork[2 * k];
        const float im  = fftWork[2 * k + 1];
        const float mag = std::sqrt (re * re + im * im);
        const float phs = std::atan2 (im, re);

        // Phase difference from previous hop
        float dPhi = phs - ch.lastPhs[k];
        ch.lastPhs[k] = phs;

        // Remove the expected phase advance for bin k
        dPhi -= twoPi * (float)k * (float)kHopSize / (float)kFFTSize;

        // Wrap to [-pi, pi]
        dPhi -= twoPi * std::round (dPhi / twoPi);

        // True frequency in fractional bins
        const float trueFreq = (float)k
                               + dPhi * (float)kFFTSize / (twoPi * (float)kHopSize);

        // Map to destination bin
        const int destK = juce::roundToInt (trueFreq * pitchRatio);
        if (destK >= 0 && destK < kNumBins)
        {
            // Accumulate magnitude; track magnitude-weighted phase increment so that
            // when multiple source bins land on the same destK (common for downward
            // shifts) we still only apply one phase step per hop per dest bin.
            outPhsInc[destK] += mag * twoPi * trueFreq * pitchRatio
                                 * (float)kHopSize / (float)kFFTSize;
            outMag[destK] += mag;
        }
    }

    // Apply exactly one phase increment per dest bin (magnitude-weighted average).
    for (int k = 0; k < kNumBins; ++k)
        if (outMag[k] > 0.0f)
            ch.synthPhs[k] += outPhsInc[k] / outMag[k];

    // ---- Synthesis: build spectrum + inverse FFT ---------------------------
    std::fill (fftWork, fftWork + kFFTSize * 2, 0.0f);

    for (int k = 0; k < kNumBins; ++k)
    {
        fftWork[2 * k]     = outMag[k] * std::cos (ch.synthPhs[k]);
        fftWork[2 * k + 1] = outMag[k] * std::sin (ch.synthPhs[k]);
    }

    // Mirror to negative-frequency bins so IFFT produces real output
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        fftWork[2 * (kFFTSize - k)]     =  fftWork[2 * k];
        fftWork[2 * (kFFTSize - k) + 1] = -fftWork[2 * k + 1];
    }

    fft.perform (reinterpret_cast<Complex*> (fftWork),
                 reinterpret_cast<Complex*> (fftWork), true);

    // ---- Overlap-add -------------------------------------------------------
    // Scale = 1 / (1.5 * N):  compensates for JUCE's un-normalised IFFT (×N)
    // and the OLA sum of analysis×synthesis Hann² windows at 4× overlap (= 1.5).
    const float scale = 1.0f / (1.5f * (float)kFFTSize);

    for (int i = 0; i < kFFTSize; ++i)
    {
        int pos = (ch.outReadPos + i) % (kFFTSize * 2);
        ch.outBuf[pos] += fftWork[2 * i] * window[i] * scale;
    }
}

// -------------------------------------------------------------------------
void PitchShifter::process (juce::AudioBuffer<float>& buffer, int semitones)
{
    // True bypass — no state changes, no latency
    if (semitones == 0)
    {
        prevSemitones = 0;
        return;
    }

    // When the pitch setting changes, stale synthPhs/lastPhs from the old ratio
    // would produce wrong phase estimates and fill outBuf with garbage.  Reset
    // cleanly so the new pitch ramps in from a known state.
    if (semitones != prevSemitones)
    {
        reset();
        prevSemitones = semitones;
    }

    const float pitchRatio = std::pow (2.0f, (float)semitones / 12.0f);
    const int   numCh      = std::min (buffer.getNumChannels(), 2);
    const int   numSmp     = buffer.getNumSamples();

    for (int ch = 0; ch < numCh; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        auto&  st   = channels[ch];

        for (int i = 0; i < numSmp; ++i)
        {
            // Push input into ring buffer
            st.inBuf[st.inWritePos] = data[i];
            st.inWritePos = (st.inWritePos + 1) % kFFTSize;

            // Fire a new hop every kHopSize samples
            if (++st.hopCount >= kHopSize)
            {
                st.hopCount = 0;
                processHop (st, pitchRatio);
            }

            // Pop output — clear slot after reading so OLA accumulates cleanly
            data[i] = st.outBuf[st.outReadPos];
            st.outBuf[st.outReadPos] = 0.0f;
            st.outReadPos = (st.outReadPos + 1) % (kFFTSize * 2);
        }
    }
}
