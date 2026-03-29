#include "EQProcessor.h"

const float EQProcessor::kBandFreqs[EQProcessor::kNumBands] = {
    80.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

EQProcessor::EQProcessor()
{
    for (int b = 0; b < kNumBands; ++b)
        freqHz[b] = kBandFreqs[b];
}

void EQProcessor::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels      = 1;

    for (int b = 0; b < kNumBands; ++b)
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            filters[b][ch].prepare(spec);
            filters[b][ch].reset();
        }

    updateFilters();
}

// Only recomputes filter coefficients when a gain or frequency actually changes,
// avoiding unnecessary work on every processBlock call.
void EQProcessor::update(const float gains[kNumBands], const float freqs[kNumBands])
{
    bool changed = false;
    for (int b = 0; b < kNumBands; ++b)
    {
        if (gainDb[b] != gains[b]) { gainDb[b] = gains[b]; changed = true; }
        if (freqHz[b] != freqs[b]) { freqHz[b] = freqs[b]; changed = true; }
    }

    if (changed)
        updateFilters();
}

void EQProcessor::updateFilters()
{
    if (sampleRate <= 0.0) return;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        // Band 0: Low shelf
        *filters[0][ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, freqHz[0], 0.7f,
                juce::Decibels::decibelsToGain(gainDb[0]));

        // Bands 1-6: Parametric peak filters
        static constexpr float kPeakQ = 1.4f;
        for (int b = 1; b <= 6; ++b)
        {
            *filters[b][ch].coefficients =
                *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                    sampleRate, freqHz[b], kPeakQ,
                    juce::Decibels::decibelsToGain(gainDb[b]));
        }

        // Band 7: High shelf
        *filters[7][ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, freqHz[7], 0.7f,
                juce::Decibels::decibelsToGain(gainDb[7]));
    }
}

void EQProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), kMaxChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];
            for (int b = 0; b < kNumBands; ++b)
                x = filters[b][ch].processSample(x);
            data[i] = x;
        }
    }
}
