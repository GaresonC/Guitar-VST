#include "StageProcessor.h"

StageProcessor::StageProcessor(float lowFreqHz, float midFreqHz, float highFreqHz)
{
    kLowFreq  = lowFreqHz;
    kMidFreq  = midFreqHz;
    kHighFreq = highFreqHz;
}

void StageProcessor::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels      = 1;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        lowFilter [ch].prepare(spec);  lowFilter [ch].reset();
        midFilter [ch].prepare(spec);  midFilter [ch].reset();
        highFilter[ch].prepare(spec);  highFilter[ch].reset();
        env[ch] = 0.0f;
    }

    // One-pole IIR envelope coefficients: coeff = exp(-1 / (time_s * sampleRate))
    // Gives an exponential decay that reaches ~63% in one time constant.
    const float sr_f = (float)sr;
    attackCoeff  = std::exp(-1.0f / (attackMs  * 0.001f * sr_f));
    releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sr_f));

    updateFilters();
}

void StageProcessor::update(float newLow, float newMid, float newHigh,
                             float newLowHz, float newMidHz, float newHighHz,
                             float newThresh, float newRatio,
                             float newAttackMs, float newReleaseMs,
                             float newMakeupDb, float blendPct)
{
    const bool eqChanged  = (lowGainDb  != newLow  || midGainDb  != newMid  || highGainDb  != newHigh
                           || kLowFreq  != newLowHz || kMidFreq  != newMidHz || kHighFreq  != newHighHz);
    const bool envChanged = (attackMs   != newAttackMs || releaseMs != newReleaseMs);

    kLowFreq  = newLowHz;
    kMidFreq  = newMidHz;
    kHighFreq = newHighHz;

    lowGainDb      = newLow;
    midGainDb      = newMid;
    highGainDb     = newHigh;
    compThreshDb   = newThresh;
    compRatio      = juce::jmax(1.0f, newRatio);
    compMakeupGain = juce::Decibels::decibelsToGain(newMakeupDb);
    compBlend      = juce::jlimit(0.0f, 1.0f, blendPct * 0.01f);

    if (eqChanged)
        updateFilters();

    if (envChanged && sampleRate > 0.0)
    {
        attackMs  = newAttackMs;
        releaseMs = newReleaseMs;
        const float sr_f = (float)sampleRate;
        attackCoeff  = std::exp(-1.0f / (attackMs  * 0.001f * sr_f));
        releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sr_f));
    }
}

void StageProcessor::updateFilters()
{
    if (sampleRate <= 0.0) return;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        *lowFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, kLowFreq, 0.7f,
                juce::Decibels::decibelsToGain(lowGainDb));

        *midFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, kMidFreq, 1.0f,
                juce::Decibels::decibelsToGain(midGainDb));

        *highFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, kHighFreq, 0.7f,
                juce::Decibels::decibelsToGain(highGainDb));
    }
}

// Soft-knee compressor applied sample-by-sample.
// e  — per-channel envelope state (modified in place via reference).
// Algorithm:
//   1. Rectify: track the absolute value of the input.
//   2. Envelope follow: one-pole smoother with separate attack/release coefficients.
//   3. Gain reduction: linear ratio above threshold — gr = (T + (e - T)/ratio) / e.
//      This is a hard-knee calculation; the "soft" quality comes from the slow
//      attack envelope rather than a polynomial knee region.
//   4. Multiply input by the gain reduction factor.
float StageProcessor::applyComp(float x, float& e) const
{
    const float absX  = std::abs(x);
    const float coeff = (absX > e) ? attackCoeff : releaseCoeff;
    e = coeff * e + (1.0f - coeff) * absX;

    const float threshold = juce::Decibels::decibelsToGain(compThreshDb);
    if (e > threshold)
    {
        // Gain reduction: maps envelope level to compressed level, then divides by e
        // so we get the multiplier to apply to the dry sample.
        const float gr = (threshold + (e - threshold) / compRatio)
                         / juce::jmax(e, 1e-9f);
        return x * gr;
    }
    return x;
}

void StageProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), kMaxChannels);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];
            x = lowFilter [ch].processSample(x);
            x = midFilter [ch].processSample(x);
            x = highFilter[ch].processSample(x);
            const float dryPostEQ = x;                // post-EQ signal for blend
            x = applyComp(x, env[ch]);
            x *= compMakeupGain;
            data[i] = dryPostEQ * (1.0f - compBlend) + x * compBlend;
        }
    }
}
