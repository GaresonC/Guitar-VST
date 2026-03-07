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
        ch.inWritePos  = 0;
        ch.outWritePos = 0;
        ch.outReadFrac = 0.0;
        ch.hopCount    = 0;
    }
}

// -------------------------------------------------------------------------
void PitchShifter::processHop (Channel& ch, int synthHop)
{
    using Complex = juce::dsp::Complex<float>;
    const float twoPi = juce::MathConstants<float>::twoPi;

    // ---- Analysis: window + forward FFT ------------------------------------
    for (int i = 0; i < kFFTSize; ++i)
    {
        int idx = (ch.inWritePos - kFFTSize + i + kFFTSize) % kFFTSize;
        fftWork[2 * i]     = ch.inBuf[idx] * window[i];
        fftWork[2 * i + 1] = 0.0f;
    }

    std::copy (fftWork, fftWork + kFFTSize * 2, fftIn);
    fft.perform (reinterpret_cast<Complex*> (fftIn),
                 reinterpret_cast<Complex*> (fftWork), false);

    // ---- Phase vocoder: true-frequency estimation --------------------------
    for (int k = 0; k < kNumBins; ++k)
    {
        const float re  = fftWork[2 * k];
        const float im  = fftWork[2 * k + 1];
        const float mag = std::sqrt (re * re + im * im);
        const float phs = std::atan2 (im, re);

        // Phase difference from previous hop
        float dPhi = phs - ch.lastPhs[k];
        ch.lastPhs[k] = phs;

        // Remove expected phase advance for bin k, wrap to [-pi, pi]
        dPhi -= twoPi * (float)k * (float)kHopSize / (float)kFFTSize;
        dPhi -= twoPi * std::round (dPhi / twoPi);

        // True instantaneous frequency (fractional bin units)
        const float trueFreq = (float)k
                               + dPhi * (float)kFFTSize / (twoPi * (float)kHopSize);

        // Accumulate synthesis phase at the synthesis hop rate (no bin remapping)
        ch.synthPhs[k] += twoPi * trueFreq * (float)synthHop / (float)kFFTSize;

        // Overwrite fftWork with synthesis spectrum in-place for IFFT
        fftWork[2 * k]     = mag * std::cos (ch.synthPhs[k]);
        fftWork[2 * k + 1] = mag * std::sin (ch.synthPhs[k]);
    }

    // ---- Synthesis: mirror + inverse FFT -----------------------------------
    for (int k = 1; k < kFFTSize / 2; ++k)
    {
        fftWork[2 * (kFFTSize - k)]     =  fftWork[2 * k];
        fftWork[2 * (kFFTSize - k) + 1] = -fftWork[2 * k + 1];
    }

    std::copy (fftWork, fftWork + kFFTSize * 2, fftIn);
    fft.perform (reinterpret_cast<Complex*> (fftIn),
                 reinterpret_cast<Complex*> (fftWork), true);

    // ---- Overlap-add into circular outBuf ----------------------------------
    // scale = synthHop / (0.375 * N^2)
    //   - divide by N: cancels JUCE unnormalized IFFT (x N)
    //   - divide by (0.375 * N / synthHop): Hann^2 OLA gain at stride synthHop
    const float scale = (float)synthHop / (0.375f * (float)kFFTSize * (float)kFFTSize);

    for (int i = 0; i < kFFTSize; ++i)
    {
        int pos = (ch.outWritePos + i) % kOutBufSize;
        ch.outBuf[pos] += fftWork[2 * i] * window[i] * scale;
    }

    ch.outWritePos = (ch.outWritePos + synthHop) % kOutBufSize;
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

    // Reset on pitch change to avoid stale phase state producing garbage output
    if (semitones != prevSemitones)
    {
        reset();
        prevSemitones = semitones;
    }

    const float pitchRatio = std::pow (2.0f, (float)semitones / 12.0f);
    const int   synthHop   = juce::roundToInt (pitchRatio * (float)kHopSize);
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

            // Fire a new hop every kHopSize input samples
            if (++st.hopCount >= kHopSize)
            {
                st.hopCount = 0;
                processHop (st, synthHop);
            }

            // Read output via linear interpolation (resampling at rate pitchRatio)
            const int    i0  = (int)st.outReadFrac % kOutBufSize;
            const int    i1  = (i0 + 1) % kOutBufSize;
            const float  frac = (float)(st.outReadFrac - std::floor (st.outReadFrac));
            data[i] = st.outBuf[i0] + frac * (st.outBuf[i1] - st.outBuf[i0]);

            // Clear consumed slot to keep OLA accumulator clean
            st.outBuf[i0] = 0.0f;

            st.outReadFrac += pitchRatio;
            if (st.outReadFrac >= (double)kOutBufSize)
                st.outReadFrac -= (double)kOutBufSize;
        }
    }
}
