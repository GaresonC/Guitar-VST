#include "AmpProcessor.h"

AmpProcessor::AmpProcessor() {}

void AmpProcessor::prepare(double sr, int samplesPerBlock)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sr;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels      = 1;

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        bassFilter    [ch].prepare(spec);
        midFilter     [ch].prepare(spec);
        trebleFilter  [ch].prepare(spec);
        presenceFilter[ch].prepare(spec);
        dcBlockFilter [ch].prepare(spec);

        bassFilter    [ch].reset();
        midFilter     [ch].reset();
        trebleFilter  [ch].reset();
        presenceFilter[ch].reset();
        dcBlockFilter [ch].reset();
    }

    updateFilters();
}

void AmpProcessor::update(float newGain, float newBass, float newMid,
                           float newTreble, float newPresence, float newMaster,
                           int newChannel)
{
    const bool filtersChanged = (bassDb != newBass || midDb != newMid ||
                                  trebleDb != newTreble || presenceDb != newPresence);

    inputGainDb    = newGain;
    bassDb         = newBass;
    midDb          = newMid;
    trebleDb       = newTreble;
    presenceDb     = newPresence;
    masterVolumeDb = newMaster;
    channel        = newChannel;

    if (filtersChanged)
        updateFilters();
}

void AmpProcessor::updateFilters()
{
    if (sampleRate <= 0.0) return;

    const float bassGain     = juce::Decibels::decibelsToGain(bassDb);
    const float midGain      = juce::Decibels::decibelsToGain(midDb);
    const float trebleGain   = juce::Decibels::decibelsToGain(trebleDb);
    const float presenceGain = juce::Decibels::decibelsToGain(presenceDb);

    for (int ch = 0; ch < kMaxChannels; ++ch)
    {
        *bassFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 250.0, 1.0, bassGain);

        *midFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, 750.0, 1.5, midGain);

        *trebleFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 3000.0, 1.0, trebleGain);

        *presenceFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 6000.0, 1.0, presenceGain);

        *dcBlockFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 10.0, 0.7);
    }
}

void AmpProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), kMaxChannels);

    for (int ch = 0; ch < numChannels; ++ch)
        processChannel(buffer.getWritePointer(ch), numSamples, ch);
}

void AmpProcessor::processChannel(float* data, int numSamples, int ch)
{
    const float inputGain  = juce::Decibels::decibelsToGain(inputGainDb);
    const float masterGain = juce::Decibels::decibelsToGain(masterVolumeDb);

    float driveGain;
    switch (channel)
    {
        case 1:  driveGain =  6.0f; break;  // Crunch
        case 2:  driveGain = 15.0f; break;  // Lead
        default: driveGain =  2.0f; break;  // Clean
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i] * inputGain;

        // Pre-amp first clipping stage
        x = softClip(x, driveGain * 0.5f);

        // Tone stack
        x = bassFilter    [ch].processSample(x);
        x = midFilter     [ch].processSample(x);
        x = trebleFilter  [ch].processSample(x);

        // Second clipping stage (crunch / lead)
        if (channel > 0)
            x = softClip(x, driveGain);

        // DC block (removes offset after heavy clipping)
        x = dcBlockFilter[ch].processSample(x);

        // Presence (power amp stage)
        x = presenceFilter[ch].processSample(x);

        // Master volume
        data[i] = x * masterGain;
    }
}

float AmpProcessor::softClip(float x, float drive)
{
    if (drive < 0.001f) return x;
    const float denom = std::tanh(drive);
    if (denom < 1e-9f) return x;
    return std::tanh(x * drive) / denom;
}
