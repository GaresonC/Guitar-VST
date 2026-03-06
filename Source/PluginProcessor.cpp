#include "PluginProcessor.h"
#include "PluginEditor.h"

static const juce::String kDefaultIRDir    = "C:/Users/Gary/Documents/Cab IR/Custom IRs/";
static const juce::String kDefaultIRFile   = "Impact Studios_IR 1.wav";
static const juce::String kDefaultAmpModel = "d:/Repos/Guitar-VST/Models/6505Plus_Red_DirectOut.json";

GuitarAmpAudioProcessor::GuitarAmpAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::mono(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

GuitarAmpAudioProcessor::~GuitarAmpAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
GuitarAmpAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"inputChannel", 1}, "Input Channel",
        juce::StringArray{"1","2","3","4","5","6","7","8"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Input Gain",
        juce::NormalisableRange<float>(0.0f, 60.0f), 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterVolume", 1}, "Master Volume",
        juce::NormalisableRange<float>(-40.0f, 0.0f), -6.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"irEnabled", 1}, "IR Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"muteEnabled", 1}, "Mute", false));

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
        juce::NormalisableRange<float>(-60.0f, 0.0f), -20.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompRatio",  1}, "Pre Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f), 3.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompAttack", 1}, "Pre Comp Attack",
        juce::NormalisableRange<float>(1.0f, 200.0f), 10.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompRelease", 1}, "Pre Comp Release",
        juce::NormalisableRange<float>(10.0f, 2000.0f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompMakeup", 1}, "Pre Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 20.0f), 0.0f));

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
        juce::NormalisableRange<float>(-60.0f, 0.0f), -18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompRatio",  1}, "Post Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f), 4.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompAttack", 1}, "Post Comp Attack",
        juce::NormalisableRange<float>(1.0f, 200.0f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompRelease", 1}, "Post Comp Release",
        juce::NormalisableRange<float>(10.0f, 2000.0f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompMakeup", 1}, "Post Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 20.0f), 0.0f));

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

    neuralAmp.prepare(sampleRate, samplesPerBlock);
    preAmpStage.prepare(sampleRate, samplesPerBlock);
    postAmpStage.prepare(sampleRate, samplesPerBlock);
    tuner.prepare(sampleRate, samplesPerBlock);
    irLoader.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    eqProcessor.prepare(sampleRate, samplesPerBlock);

    // Apply current parameter values on prepare
    neuralAmp.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("masterVolume")->load());

    // Load default amp model if none is currently loaded
    if (!neuralAmp.hasModel())
        neuralAmp.loadModel(juce::File(kDefaultAmpModel));

    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);

    // Load default IR if none is currently loaded
    if (!irLoader.hasLoadedIR())
    {
        juce::File defaultIR(kDefaultIRDir + kDefaultIRFile);
        irLoader.loadIR(defaultIR);
    }
}

void GuitarAmpAudioProcessor::releaseResources() {}

bool GuitarAmpAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Output must be mono or stereo
    auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    // Input: accept 1–8 channels so standalone shows all device inputs individually
    auto numIn = layouts.getMainInputChannelSet().size();
    if (numIn < 1 || numIn > 8)
        return false;

    return true;
}

void GuitarAmpAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    // Route the user-selected input channel to channel 0 for mono processing
    const int selectedCh = juce::roundToInt(
        apvts.getRawParameterValue("inputChannel")->load()); // 0-indexed (0 = Input 1)
    if (selectedCh > 0 && selectedCh < numIn)
        buffer.copyFrom(0, 0, buffer, selectedCh, 0, buffer.getNumSamples());

    // Zero all channels above 0 — we process mono through ch0
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Tuner always runs on the dry signal before any processing
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
        apvts.getRawParameterValue("preCompRatio")->load(),
        apvts.getRawParameterValue("preCompAttack")->load(),
        apvts.getRawParameterValue("preCompRelease")->load(),
        apvts.getRawParameterValue("preCompMakeup")->load());
    preAmpStage.process(buffer);

    // Neural amp processing
    neuralAmp.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("masterVolume")->load());
    neuralAmp.process(buffer);

    // Post-amp stage: EQ + compression to glue and shape after distortion
    postAmpStage.update(
        apvts.getRawParameterValue("postEqLow")->load(),
        apvts.getRawParameterValue("postEqMid")->load(),
        apvts.getRawParameterValue("postEqHigh")->load(),
        apvts.getRawParameterValue("postCompThresh")->load(),
        apvts.getRawParameterValue("postCompRatio")->load(),
        apvts.getRawParameterValue("postCompAttack")->load(),
        apvts.getRawParameterValue("postCompRelease")->load(),
        apvts.getRawParameterValue("postCompMakeup")->load());
    postAmpStage.process(buffer);

    // IR / Cabinet convolution
    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);
    irLoader.process(buffer);

    // Copy processed ch0 to ch1 for stereo output
    if (numOut >= 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

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

    // Mute — silence output while keeping tuner active
    if (apvts.getRawParameterValue("muteEnabled")->load() > 0.5f)
        buffer.clear();
}

//==============================================================================
void GuitarAmpAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("irFilePath",    irLoader.getFilePath(),        nullptr);
    state.setProperty("ampModelPath",  neuralAmp.getModelFilePath(),  nullptr);
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

        juce::String ampModelPath = apvts.state.getProperty("ampModelPath", "");
        if (ampModelPath.isNotEmpty())
            neuralAmp.loadModel(juce::File(ampModelPath));
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
