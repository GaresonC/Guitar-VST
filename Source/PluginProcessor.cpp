#include "PluginProcessor.h"
#include "PluginEditor.h"

static const juce::String kDefaultIRDir  = "C:/Users/Gary/Documents/Cab IR/Custom IRs/";
static const juce::String kDefaultIRFile = "Impact Studios_IR 1.wav";

GuitarAmpAudioProcessor::GuitarAmpAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

GuitarAmpAudioProcessor::~GuitarAmpAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
GuitarAmpAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Input Gain",
        juce::NormalisableRange<float>(0.0f, 60.0f), 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"bass", 1}, "Bass",
        juce::NormalisableRange<float>(-15.0f, 15.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mid", 1}, "Mid",
        juce::NormalisableRange<float>(-15.0f, 15.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"treble", 1}, "Treble",
        juce::NormalisableRange<float>(-15.0f, 15.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"presence", 1}, "Presence",
        juce::NormalisableRange<float>(-10.0f, 10.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterVolume", 1}, "Master Volume",
        juce::NormalisableRange<float>(-40.0f, 0.0f), -6.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"irEnabled", 1}, "IR Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"tunerEnabled", 1}, "Tuner Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"noiseGateEnabled", 1}, "Gate Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseGateThreshold", 1}, "Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, -20.0f), -60.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"pitchShiftEnabled", 1}, "Pitch Shift Enabled", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitchShiftSemitones", 1}, "Pitch Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 1.0f), 0.0f));

    // Pre-amp stage: 3-band EQ + compressor (shapes signal before distortion)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqLow",  1}, "Pre EQ Low",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqMid",  1}, "Pre EQ Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqHigh", 1}, "Pre EQ High",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompThresh", 1}, "Pre Comp Thresh",
        juce::NormalisableRange<float>(-60.0f, 0.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompRatio",  1}, "Pre Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f), 3.0f));

    // Post-amp stage: 3-band EQ + compressor (shapes and glues after distortion)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqLow",  1}, "Post EQ Low",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqMid",  1}, "Post EQ Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqHigh", 1}, "Post EQ High",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompThresh", 1}, "Post Comp Thresh",
        juce::NormalisableRange<float>(-60.0f, 0.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompRatio",  1}, "Post Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f), 4.0f));

    // 8-band post-IR EQ (±15 dB each band)
    static const char* eqParamIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    static const char* eqParamNames[EQProcessor::kNumBands] = {
        "EQ 80Hz","EQ 250Hz","EQ 500Hz","EQ 1kHz",
        "EQ 2kHz","EQ 4kHz","EQ 8kHz","EQ 16kHz"
    };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{eqParamIds[b], 1}, eqParamNames[b],
            juce::NormalisableRange<float>(-15.0f, 15.0f), 0.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void GuitarAmpAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32)juce::jmax(1, samplesPerBlock);
    spec.numChannels      = (juce::uint32)juce::jmax(1, getTotalNumInputChannels());

    noiseGate.prepare(spec);
    noiseGate.setThreshold(apvts.getRawParameterValue("noiseGateThreshold")->load());
    noiseGate.setAttack(10.0f);
    noiseGate.setRelease(150.0f);
    noiseGate.setRatio(10.0f);

    pitchShifter.prepare(sampleRate, samplesPerBlock);

    ampProcessor.prepare(sampleRate, samplesPerBlock);
    preAmpStage.prepare(sampleRate, samplesPerBlock);
    postAmpStage.prepare(sampleRate, samplesPerBlock);
    tuner.prepare(sampleRate, samplesPerBlock);
    irLoader.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    eqProcessor.prepare(sampleRate, samplesPerBlock);

    // Apply current parameter values on prepare
    ampProcessor.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("bass")->load(),
        apvts.getRawParameterValue("mid")->load(),
        apvts.getRawParameterValue("treble")->load(),
        apvts.getRawParameterValue("presence")->load(),
        apvts.getRawParameterValue("masterVolume")->load());

    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);

    // Load default IR if none is currently loaded
    if (!irLoader.hasLoadedIR())
    {
        juce::File defaultIR(kDefaultIRDir + kDefaultIRFile);
        irLoader.loadIR(defaultIR);
    }
}

void GuitarAmpAudioProcessor::releaseResources() {}

void GuitarAmpAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that aren't mapped to an input
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Tuner runs on the dry signal (channel 0) before any processing
    if (apvts.getRawParameterValue("tunerEnabled")->load() > 0.5f)
        tuner.process(buffer);

    // Noise gate (pre-amp)
    noiseGate.setThreshold(apvts.getRawParameterValue("noiseGateThreshold")->load());
    if (apvts.getRawParameterValue("noiseGateEnabled")->load() > 0.5f)
    {
        juce::dsp::AudioBlock<float>              block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        noiseGate.process(ctx);
    }

    // Pitch shifter (after gate, before amp — clean signal)
    if (apvts.getRawParameterValue("pitchShiftEnabled")->load() > 0.5f)
    {
        const int semitones = juce::roundToInt(
            apvts.getRawParameterValue("pitchShiftSemitones")->load());
        pitchShifter.process(buffer, semitones);
    }

    // Pre-amp stage: EQ + compression before distortion
    preAmpStage.update(
        apvts.getRawParameterValue("preEqLow")->load(),
        apvts.getRawParameterValue("preEqMid")->load(),
        apvts.getRawParameterValue("preEqHigh")->load(),
        apvts.getRawParameterValue("preCompThresh")->load(),
        apvts.getRawParameterValue("preCompRatio")->load());
    preAmpStage.process(buffer);

    // Amp processing
    ampProcessor.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("bass")->load(),
        apvts.getRawParameterValue("mid")->load(),
        apvts.getRawParameterValue("treble")->load(),
        apvts.getRawParameterValue("presence")->load(),
        apvts.getRawParameterValue("masterVolume")->load());
    ampProcessor.process(buffer);

    // Post-amp stage: EQ + compression to glue and shape after distortion
    postAmpStage.update(
        apvts.getRawParameterValue("postEqLow")->load(),
        apvts.getRawParameterValue("postEqMid")->load(),
        apvts.getRawParameterValue("postEqHigh")->load(),
        apvts.getRawParameterValue("postCompThresh")->load(),
        apvts.getRawParameterValue("postCompRatio")->load());
    postAmpStage.process(buffer);

    // IR / Cabinet convolution
    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);
    irLoader.process(buffer);

    // Post-IR 8-band EQ
    static const char* eqParamIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    float eqGains[EQProcessor::kNumBands];
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        eqGains[b] = apvts.getRawParameterValue(eqParamIds[b])->load();
    eqProcessor.update(eqGains);
    eqProcessor.process(buffer);
}

//==============================================================================
void GuitarAmpAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("irFilePath", irLoader.getFilePath(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GuitarAmpAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(newState);

        juce::String irPath = apvts.state.getProperty("irFilePath", "");
        if (irPath.isNotEmpty())
            irLoader.loadIR(juce::File(irPath));
    }
}

//==============================================================================
juce::AudioProcessorEditor* GuitarAmpAudioProcessor::createEditor()
{
    return new GuitarAmpAudioProcessorEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarAmpAudioProcessor();
}
