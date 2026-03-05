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
        interstageHp3Filter[ch].prepare(spec);
        bassFilter         [ch].prepare(spec);
        midFilter          [ch].prepare(spec);
        trebleFilter       [ch].prepare(spec);
        postDistLpFilter   [ch].prepare(spec);
        presenceFilter     [ch].prepare(spec);
        dcBlockFilter      [ch].prepare(spec);

        preHpFilter        [ch].reset();
        interstageHp1Filter[ch].reset();
        interstageHp2Filter[ch].reset();
        interstageHp3Filter[ch].reset();
        bassFilter         [ch].reset();
        midFilter          [ch].reset();
        trebleFilter       [ch].reset();
        postDistLpFilter   [ch].reset();
        presenceFilter     [ch].reset();
        dcBlockFilter      [ch].reset();

        envFollower[ch] = 0.0f;
    }

    // Compressor: 3 ms attack (tight, Nameless-style), 120 ms release
    const float sr_f = (float)sr;
    compAttackCoeff  = std::exp(-1.0f / (0.003f * sr_f));
    compReleaseCoeff = std::exp(-1.0f / (0.120f * sr_f));

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
        // Pre-distortion HP: 100 Hz — tight Nameless-style low end, no flub
        *preHpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 100.0f, 0.7f);

        // Interstage coupling capacitor simulations — 150 Hz, removes sub buildup
        *interstageHp1Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 150.0f, 0.7f);

        *interstageHp2Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 150.0f, 0.7f);

        *interstageHp3Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 150.0f, 0.7f);

        // Tone stack — tuned for Nameless character:
        // Bass: tight shelf at 120 Hz (very focused low end)
        *bassFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 120.0f, 0.7f, bassGain);

        // Mid: broad peak/cut at 550 Hz (Nameless scooped-mid character)
        *midFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, 550.0f, 0.9f, midGain);

        // Treble: shelf at 4500 Hz (bright, articulate attack)
        *trebleFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 4500.0f, 0.7f, trebleGain);

        // Post-distortion LP: 9500 Hz — bright but not piercing
        *postDistLpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, 9500.0f, 0.7f);

        // Presence (power-amp): 7000 Hz — upper midrange clarity
        *presenceFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 7000.0f, 0.9f, presenceGain);

        *dcBlockFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 10.0f, 0.7f);
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

    // Drive levels tuned for Fortin Nameless character:
    // Lead: 4 cascaded stages for aggressive, compressed saturation
    float stage1Drive, stage2Drive, stage3Drive, stage4Drive;
    switch (channel)
    {
        case 1:  // Crunch
            stage1Drive =  6.0f;
            stage2Drive = 11.0f;
            stage3Drive =  0.0f;
            stage4Drive =  0.0f;
            break;
        case 2:  // Lead — Nameless-style 4-stage high gain
            stage1Drive =  9.0f;
            stage2Drive = 16.0f;
            stage3Drive = 22.0f;
            stage4Drive =  6.0f;
            break;
        default: // Clean — single mild stage
            stage1Drive =  1.5f;
            stage2Drive =  0.0f;
            stage3Drive =  0.0f;
            stage4Drive =  0.0f;
            break;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Pre-HP: removes sub-bass before distortion (tight Nameless low end)
        x = preHpFilter[ch].processSample(x);

        x *= inputGain;

        // Stage 1: symmetric soft clip
        x = softClip(x, stage1Drive);

        // Coupling cap between stages 1→2
        x = interstageHp1Filter[ch].processSample(x);

        // Stage 2: asymmetric clip — even harmonics, tube-like
        if (channel >= 1)
            x = asymClip(x, stage2Drive);

        // Stages 3 & 4: lead only (4-stage Nameless saturation)
        if (channel >= 2)
        {
            x = interstageHp2Filter[ch].processSample(x);
            x = softClip(x, stage3Drive);

            x = interstageHp3Filter[ch].processSample(x);
            x = asymClip(x, stage4Drive);  // output stage character
        }

        // Tone stack
        x = bassFilter  [ch].processSample(x);
        x = midFilter   [ch].processSample(x);
        x = trebleFilter[ch].processSample(x);

        // DC block (removes offset after asymmetric clipping)
        x = dcBlockFilter[ch].processSample(x);

        // Post-distortion LP — removes fizz/aliasing
        x = postDistLpFilter[ch].processSample(x);

        // Tight compressor on driven channels
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
    const float d = (x >= 0.0f) ? drive : drive * 0.75f;
    return std::tanh(x * d) / denom;
}

// Soft-knee feedforward compressor using an envelope follower.
float AmpProcessor::applyCompression(float x, float& env) const
{
    const float absX  = std::abs(x);
    const float coeff = (absX > env) ? compAttackCoeff : compReleaseCoeff;
    env = coeff * env + (1.0f - coeff) * absX;

    constexpr float threshold = 0.40f;
    constexpr float ratio     = 4.0f;

    if (env > threshold)
    {
        const float gainReduction = (threshold + (env - threshold) / ratio)
                                    / juce::jmax(env, 1e-9f);
        return x * gainReduction;
    }
    return x;
}
