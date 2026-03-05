#pragma once
#include <JuceHeader.h>

class AmpProcessor
{
public:
    AmpProcessor();

    void prepare(double sampleRate, int samplesPerBlock);
    void process(juce::AudioBuffer<float>& buffer);

    // Called once per processBlock with all current parameter values
    void update(float inputGainDb, float bassDb, float midDb,
                float trebleDb, float presenceDb, float masterVolumeDb,
                int channel);

private:
    static constexpr int kMaxChannels = 2;

    double sampleRate = 44100.0;

    float inputGainDb   =  20.0f;
    float bassDb        =   0.0f;
    float midDb         =   0.0f;
    float trebleDb      =   0.0f;
    float presenceDb    =   0.0f;
    float masterVolumeDb = -6.0f;
    int   channel       =   0;

    // One set of IIR filters per channel (single-channel filter instances)
    juce::dsp::IIR::Filter<float> preHpFilter        [kMaxChannels];
    juce::dsp::IIR::Filter<float> interstageHp1Filter[kMaxChannels];
    juce::dsp::IIR::Filter<float> interstageHp2Filter[kMaxChannels];
    juce::dsp::IIR::Filter<float> interstageHp3Filter[kMaxChannels];
    juce::dsp::IIR::Filter<float> bassFilter         [kMaxChannels];
    juce::dsp::IIR::Filter<float> midFilter          [kMaxChannels];
    juce::dsp::IIR::Filter<float> trebleFilter       [kMaxChannels];
    juce::dsp::IIR::Filter<float> postDistLpFilter   [kMaxChannels];
    juce::dsp::IIR::Filter<float> presenceFilter     [kMaxChannels];
    juce::dsp::IIR::Filter<float> dcBlockFilter      [kMaxChannels];

    // Compressor state
    float envFollower[kMaxChannels] = { 0.0f, 0.0f };
    float compAttackCoeff  = 0.0f;
    float compReleaseCoeff = 0.0f;

    void updateFilters();
    void processChannel(float* data, int numSamples, int ch);

    static float softClip(float x, float drive);
    static float asymClip(float x, float drive);
    float applyCompression(float x, float& env) const;
};
