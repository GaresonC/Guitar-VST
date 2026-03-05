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
        preHpFilter        [ch].prepare(spec);
        interstageHp1Filter[ch].prepare(spec);
        interstageHp2Filter[ch].prepare(spec);
        bassFilter         [ch].prepare(spec);
        midFilter          [ch].prepare(spec);
        trebleFilter       [ch].prepare(spec);
        postDistLpFilter   [ch].prepare(spec);
        presenceFilter     [ch].prepare(spec);
        dcBlockFilter      [ch].prepare(spec);

        preHpFilter        [ch].reset();
        interstageHp1Filter[ch].reset();
        interstageHp2Filter[ch].reset();
        bassFilter         [ch].reset();
        midFilter          [ch].reset();
        trebleFilter       [ch].reset();
        postDistLpFilter   [ch].reset();
        presenceFilter     [ch].reset();
        dcBlockFilter      [ch].reset();

        envFollower[ch] = 0.0f;
    }

    // Compressor: 5 ms attack, 150 ms release
    const float sr_f = (float)sr;
    compAttackCoeff  = std::exp(-1.0f / (0.005f * sr_f));
    compReleaseCoeff = std::exp(-1.0f / (0.150f * sr_f));

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
        // Pre-distortion: remove sub-bass to prevent muddy low-end clipping
        *preHpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 80.0, 0.7);

        // Interstage coupling capacitor simulation (removes low-frequency buildup between stages)
        *interstageHp1Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 120.0, 0.7);

        *interstageHp2Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 120.0, 0.7);

        // Tone stack
        *bassFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 250.0, 0.7, bassGain);

        *midFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, 750.0, 1.5, midGain);

        *trebleFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 3500.0, 0.7, trebleGain);

        // Post-distortion low-pass: tames fizz and aliasing harshness
        *postDistLpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, 7000.0, 0.7);

        // Presence (power-amp stage high shelf)
        *presenceFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 5000.0, 0.9, presenceGain);

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

    float stage1Drive, stage2Drive, stage3Drive;
    switch (channel)
    {
        case 1:  // Crunch — two stages, asymmetric second stage for even harmonics
            stage1Drive =  5.0f;
            stage2Drive =  9.0f;
            stage3Drive =  0.0f;
            break;
        case 2:  // Lead — three stages for full saturation
            stage1Drive =  7.0f;
            stage2Drive = 13.0f;
            stage3Drive = 18.0f;
            break;
        default: // Clean — single mild stage
            stage1Drive =  1.5f;
            stage2Drive =  0.0f;
            stage3Drive =  0.0f;
            break;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Remove sub-bass before distortion to prevent flubby low-end
        x = preHpFilter[ch].processSample(x);

        x *= inputGain;

        // Stage 1: symmetric soft clip (all channels)
        x = softClip(x, stage1Drive);

        // Coupling capacitor between stages (removes DC/sub buildup)
        x = interstageHp1Filter[ch].processSample(x);

        // Stage 2: asymmetric clip (crunch + lead) — generates even harmonics
        if (channel >= 1)
            x = asymClip(x, stage2Drive);

        // Stage 3: final saturation stage (lead only)
        if (channel >= 2)
        {
            x = interstageHp2Filter[ch].processSample(x);
            x = softClip(x, stage3Drive);
        }

        // Tone stack
        x = bassFilter [ch].processSample(x);
        x = midFilter  [ch].processSample(x);
        x = trebleFilter[ch].processSample(x);

        // DC block (removes offset after asymmetric clipping)
        x = dcBlockFilter[ch].processSample(x);

        // Post-distortion low-pass — removes harshness / fizz
        x = postDistLpFilter[ch].processSample(x);

        // Compressor on driven channels to control dynamics
        if (channel > 0)
            x = applyCompression(x, envFollower[ch]);

        // Presence (power-amp stage)
        x = presenceFilter[ch].processSample(x);

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

// Asymmetric clip: positive half clips harder than negative, generating even harmonics
// like a real triode tube stage biased toward cut-off.
float AmpProcessor::asymClip(float x, float drive)
{
    if (drive < 0.001f) return x;
    const float denom = std::tanh(drive);
    if (denom < 1e-9f) return x;
    // Positive half: full drive (harder saturation)
    // Negative half: 75% drive (softer — creates the asymmetry)
    const float d = (x >= 0.0f) ? drive : drive * 0.75f;
    return std::tanh(x * d) / denom;
}

// Soft-knee feedforward compressor using an envelope follower.
float AmpProcessor::applyCompression(float x, float& env) const
{
    const float absX = std::abs(x);
    const float coeff = (absX > env) ? compAttackCoeff : compReleaseCoeff;
    env = coeff * env + (1.0f - coeff) * absX;

    constexpr float threshold = 0.45f;
    constexpr float ratio     = 3.5f;

    if (env > threshold)
    {
        const float gainReduction = (threshold + (env - threshold) / ratio)
                                    / juce::jmax(env, 1e-9f);
        return x * gainReduction;
    }
    return x;
}
