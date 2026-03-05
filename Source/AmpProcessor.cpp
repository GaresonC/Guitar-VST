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

    // Compressor: 1 ms attack (very tight, 5150 palm-mute response), 80 ms release
    const float sr_f = (float)sr;
    compAttackCoeff  = std::exp(-1.0f / (0.001f * sr_f));
    compReleaseCoeff = std::exp(-1.0f / (0.080f * sr_f));

    updateFilters();
}

void AmpProcessor::update(float newGain, float newBass, float newMid,
                           float newTreble, float newPresence, float newMaster)
{
    const bool filtersChanged = (bassDb != newBass || midDb != newMid ||
                                  trebleDb != newTreble || presenceDb != newPresence);

    inputGainDb    = newGain;
    bassDb         = newBass;
    midDb          = newMid;
    trebleDb       = newTreble;
    presenceDb     = newPresence;
    masterVolumeDb = newMaster;

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
        // Pre-distortion HP: 75 Hz — 5150's characteristically tight, controlled low end
        *preHpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 75.0f, 0.7f);

        // Interstage coupling cap simulations — 180 Hz, tighter than a Nameless,
        // reinforces the 5150's mid-focused aggression
        *interstageHp1Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 180.0f, 0.7f);

        *interstageHp2Filter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighPass(
                sampleRate, 180.0f, 0.7f);

        // Tone stack — tuned for Peavey 5150 character:
        // Bass: tight punch shelf at 100 Hz
        *bassFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
                sampleRate, 100.0f, 0.7f, bassGain);

        // Mid: peak/cut at 750 Hz, Q=0.9 — classic 5150 scooped-mid response
        *midFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                sampleRate, 750.0f, 0.9f, midGain);

        // Treble: shelf at 3500 Hz — bright, cutting attack without harshness
        *trebleFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 3500.0f, 0.7f, trebleGain);

        // Post-distortion LP: 8000 Hz — slightly darker rolloff than a Nameless
        *postDistLpFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, 8000.0f, 0.7f);

        // Presence (power-amp): 5500 Hz — upper harmonic clarity, 5150 cut
        *presenceFilter[ch].coefficients =
            *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
                sampleRate, 5500.0f, 0.9f, presenceGain);

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

    // Drive levels: 3-stage 5150-style crunch
    const float stage1Drive =  7.0f;
    const float stage2Drive = 13.0f;
    const float stage3Drive =  9.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float x = data[i];

        // Pre-HP: 75 Hz — removes sub-bass before distortion (tight 5150 low end)
        x = preHpFilter[ch].processSample(x);

        x *= inputGain;

        // Stage 1: asymmetric triode stage — even harmonics, tube warmth
        x = asymClip(x, stage1Drive);

        // Coupling cap between stages 1→2
        x = interstageHp1Filter[ch].processSample(x);

        // Stage 2
        x = asymClip(x, stage2Drive);

        // Stage 3
        x = interstageHp2Filter[ch].processSample(x);
        x = softClip(x, stage3Drive);

        // Tone stack
        x = bassFilter  [ch].processSample(x);
        x = midFilter   [ch].processSample(x);
        x = trebleFilter[ch].processSample(x);

        // DC block (removes offset after asymmetric clipping)
        x = dcBlockFilter[ch].processSample(x);

        // Post-distortion LP — removes fizz/aliasing
        x = postDistLpFilter[ch].processSample(x);

        // Very tight compressor — essential for 5150 palm-mute thump
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
    // 5150 tube stages have strong asymmetry — negative half runs at 65% drive
    const float d = (x >= 0.0f) ? drive : drive * 0.65f;
    return std::tanh(x * d) / denom;
}

// Soft-knee feedforward compressor using an envelope follower.
float AmpProcessor::applyCompression(float x, float& env) const
{
    const float absX  = std::abs(x);
    const float coeff = (absX > env) ? compAttackCoeff : compReleaseCoeff;
    env = coeff * env + (1.0f - coeff) * absX;

    constexpr float threshold = 0.35f;
    constexpr float ratio     = 5.0f;

    if (env > threshold)
    {
        const float gainReduction = (threshold + (env - threshold) / ratio)
                                    / juce::jmax(env, 1e-9f);
        return x * gainReduction;
    }
    return x;
}
