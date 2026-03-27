#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

static const juce::String kDefaultIRDir    = "C:/Users/Gary/Documents/Cab IR/Custom IRs/";
static const juce::String kDefaultIRFile   = "Impact Studios_IR 1.wav";

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

    // Value formatter lambdas
    auto dbFmt    = [](float v, int) { return juce::String(v, 1) + " dB"; };
    auto dbParse  = [](const juce::String& s) { return s.getFloatValue(); };
    auto msFmt    = [](float v, int) { return juce::String(juce::roundToInt(v)) + " ms"; };
    auto msParse  = [](const juce::String& s) { return s.getFloatValue(); };
    auto ratioFmt = [](float v, int) { return juce::String(v, 1) + ":1"; };
    auto ratioParse = [](const juce::String& s) { return s.getFloatValue(); };
    auto pctFmt   = [](float v, int) { return juce::String(juce::roundToInt(v)) + "%"; };
    auto pctParse = [](const juce::String& s) { return s.getFloatValue(); };
    auto hzFmt = [](float v, int) -> juce::String {
        if (v >= 1000.0f) return juce::String(v / 1000.0f, 1) + " kHz";
        return juce::String(juce::roundToInt(v)) + " Hz";
    };
    auto hzParse = [](const juce::String& s) -> float {
        if (s.containsIgnoreCase("k")) return s.getFloatValue() * 1000.0f;
        return s.getFloatValue();
    };
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputTrim", 1}, "Input Trim",
        juce::NormalisableRange<float>(-40.0f, 40.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Input Gain",
        juce::NormalisableRange<float>(-20.0f, 10.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"masterVolume", 1}, "Master Volume",
        juce::NormalisableRange<float>(-60.0f, 6.0f), -6.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"irEnabled", 1}, "IR Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"muteEnabled", 1}, "Mute", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"noiseGateEnabled", 1}, "Gate Enabled", true));

    // Section bypass parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassPreEq", 1}, "Bypass Pre EQ", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassPreComp", 1}, "Bypass Pre Comp", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassAmp", 1}, "Bypass Amp", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassPostEq", 1}, "Bypass Post EQ", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassPostComp", 1}, "Bypass Post Comp", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassMfEq", 1}, "Bypass MF EQ", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypassReverb", 1}, "Bypass Reverb", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseGateThreshold", 1}, "Gate Threshold",
        juce::NormalisableRange<float>(-100.0f, 0.0f), -60.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"outputVolume", 1}, "Output Volume",
        juce::NormalisableRange<float>(-60.0f, 12.0f), -6.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));

    // Pre-amp stage: 3-band EQ + compressor (shapes signal before distortion)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqLow",  1}, "Pre EQ Low",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqMid",  1}, "Pre EQ Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqHigh", 1}, "Pre EQ High",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqLowFreq",  1}, "Pre EQ Low Freq",
        juce::NormalisableRange<float>(60.0f, 600.0f, 0.0f, 0.4f), 200.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqMidFreq",  1}, "Pre EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 3000.0f, 0.0f, 0.4f), 700.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preEqHighFreq", 1}, "Pre EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 12000.0f, 0.0f, 0.4f), 4500.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompThresh", 1}, "Pre Comp Thresh",
        juce::NormalisableRange<float>(-80.0f, 0.0f), -20.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompRatio",  1}, "Pre Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 50.0f), 3.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        ratioFmt, ratioParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompAttack", 1}, "Pre Comp Attack",
        juce::NormalisableRange<float>(0.1f, 500.0f), 10.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        msFmt, msParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompRelease", 1}, "Pre Comp Release",
        juce::NormalisableRange<float>(1.0f, 5000.0f), 80.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        msFmt, msParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompMakeup", 1}, "Pre Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 40.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"preCompBlend", 1}, "Pre Comp Blend",
        juce::NormalisableRange<float>(0.0f, 100.0f), 100.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt, pctParse));

    // Post-amp stage: 3-band EQ + compressor (shapes and glues after distortion)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqLow",  1}, "Post EQ Low",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqMid",  1}, "Post EQ Mid",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqHigh", 1}, "Post EQ High",
        juce::NormalisableRange<float>(-12.0f, 12.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqLowFreq",  1}, "Post EQ Low Freq",
        juce::NormalisableRange<float>(60.0f, 600.0f, 0.0f, 0.4f), 150.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqMidFreq",  1}, "Post EQ Mid Freq",
        juce::NormalisableRange<float>(200.0f, 3000.0f, 0.0f, 0.4f), 1000.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postEqHighFreq", 1}, "Post EQ High Freq",
        juce::NormalisableRange<float>(2000.0f, 12000.0f, 0.0f, 0.4f), 5000.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        hzFmt, hzParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompThresh", 1}, "Post Comp Thresh",
        juce::NormalisableRange<float>(-80.0f, 0.0f), -18.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompRatio",  1}, "Post Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 50.0f), 4.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        ratioFmt, ratioParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompAttack", 1}, "Post Comp Attack",
        juce::NormalisableRange<float>(0.1f, 500.0f), 5.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        msFmt, msParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompRelease", 1}, "Post Comp Release",
        juce::NormalisableRange<float>(1.0f, 5000.0f), 80.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        msFmt, msParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompMakeup", 1}, "Post Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 40.0f), 0.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        dbFmt, dbParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"postCompBlend", 1}, "Post Comp Blend",
        juce::NormalisableRange<float>(0.0f, 100.0f), 100.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt, pctParse));

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
            juce::NormalisableRange<float>(-15.0f, 15.0f), 0.0f,
            juce::String{}, juce::AudioProcessorParameter::genericParameter,
            dbFmt, dbParse));

    // 8-band post-IR EQ frequencies
    static const char* eqFreqIds[EQProcessor::kNumBands] = {
        "eq1Freq","eq2Freq","eq3Freq","eq4Freq",
        "eq5Freq","eq6Freq","eq7Freq","eq8Freq"
    };
    static const char* eqFreqNames[EQProcessor::kNumBands] = {
        "EQ1 Freq","EQ2 Freq","EQ3 Freq","EQ4 Freq",
        "EQ5 Freq","EQ6 Freq","EQ7 Freq","EQ8 Freq"
    };
    for (int b = 0; b < EQProcessor::kNumBands; ++b)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{eqFreqIds[b], 1}, eqFreqNames[b],
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f),
            EQProcessor::kBandFreqs[b],
            juce::String{}, juce::AudioProcessorParameter::genericParameter,
            hzFmt, hzParse));

    // Reverb parameters (CloudSeed)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverbMix", 1}, "Reverb Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f), 30.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt, pctParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverbDecay", 1}, "Reverb Decay",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt, pctParse));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverbSize", 1}, "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 100.0f), 50.0f,
        juce::String{}, juce::AudioProcessorParameter::genericParameter,
        pctFmt, pctParse));

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

    neuralAmp.prepare(sampleRate, samplesPerBlock);
    preAmpStage.prepare(sampleRate, samplesPerBlock);
    postAmpStage.prepare(sampleRate, samplesPerBlock);
    tuner.prepare(sampleRate, samplesPerBlock);
    irLoader.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    eqProcessor.prepare(sampleRate, samplesPerBlock);

    // Initialize CloudSeed reverb
    reverb = std::make_unique<Cloudseed::ReverbController>((int)sampleRate);
    // Set sensible defaults for guitar use
    reverb->SetParameter(Cloudseed::Parameter::InputMix, 0.0);          // no cross-feed (mono-ish input)
    reverb->SetParameter(Cloudseed::Parameter::TapEnabled, 1.0);        // enable early reflections
    reverb->SetParameter(Cloudseed::Parameter::TapCount, 0.3);          // moderate tap count
    reverb->SetParameter(Cloudseed::Parameter::TapDecay, 0.8);          // early decay
    reverb->SetParameter(Cloudseed::Parameter::TapPredelay, 0.05);      // short predelay
    reverb->SetParameter(Cloudseed::Parameter::TapLength, 0.3);         // moderate tap spread
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseEnabled, 1.0);
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseCount, 0.5); // 6 stages
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseDelay, 0.3);
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseModAmount, 0.2);
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseModRate, 0.3);
    reverb->SetParameter(Cloudseed::Parameter::EarlyDiffuseFeedback, 0.6);
    reverb->SetParameter(Cloudseed::Parameter::LateDiffuseEnabled, 1.0);
    reverb->SetParameter(Cloudseed::Parameter::LateDiffuseCount, 0.5);
    reverb->SetParameter(Cloudseed::Parameter::LateLineCount, 0.7);     // 8 lines
    reverb->SetParameter(Cloudseed::Parameter::LateLineModAmount, 0.15);
    reverb->SetParameter(Cloudseed::Parameter::LateLineModRate, 0.25);
    reverb->SetParameter(Cloudseed::Parameter::LateDiffuseModAmount, 0.15);
    reverb->SetParameter(Cloudseed::Parameter::LateDiffuseModRate, 0.25);
    reverb->SetParameter(Cloudseed::Parameter::LateDiffuseFeedback, 0.5);
    reverb->SetParameter(Cloudseed::Parameter::EqLowShelfEnabled, 1.0);
    reverb->SetParameter(Cloudseed::Parameter::EqHighShelfEnabled, 1.0);
    reverb->SetParameter(Cloudseed::Parameter::EqLowpassEnabled, 1.0);
    reverb->SetParameter(Cloudseed::Parameter::EqLowFreq, 0.3);
    reverb->SetParameter(Cloudseed::Parameter::EqHighFreq, 0.5);
    reverb->SetParameter(Cloudseed::Parameter::EqCutoff, 0.7);
    reverb->SetParameter(Cloudseed::Parameter::EqLowGain, 0.4);
    reverb->SetParameter(Cloudseed::Parameter::EqHighGain, 0.3);
    reverb->SetParameter(Cloudseed::Parameter::DryOut, 1.0);            // full dry
    reverb->SetParameter(Cloudseed::Parameter::EarlyOut, 0.7);          // moderate early
    reverb->SetParameter(Cloudseed::Parameter::LateOut, 0.7);           // moderate late
    // Apply user params
    {
        float mix   = apvts.getRawParameterValue("reverbMix")->load() / 100.0f;
        float decay = apvts.getRawParameterValue("reverbDecay")->load() / 100.0f;
        float size  = apvts.getRawParameterValue("reverbSize")->load() / 100.0f;
        reverb->SetParameter(Cloudseed::Parameter::LateLineDecay, decay);
        reverb->SetParameter(Cloudseed::Parameter::LateLineSize, size);
        (void)mix; // mix is applied in processBlock as dry/wet blend
    }

    // Apply current parameter values on prepare
    neuralAmp.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("masterVolume")->load());

    // Load default amp model if none is currently loaded
    if (!neuralAmp.hasModel())
    {
        int modelSize = 0;
        const char* modelData = BinaryData::getNamedResource ("6505Plus_Red_DirectOut_json", modelSize);
        if (modelData && modelSize > 0)
            neuralAmp.loadModel (modelData, modelSize);
    }

    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);

    // Load default IR if none is currently loaded — use first bundled IR
    if (!irLoader.hasLoadedIR())
    {
        // Try the old file-based default first, fall back to first bundled IR
        juce::File defaultIR(kDefaultIRDir + kDefaultIRFile);
        if (!irLoader.loadIR(defaultIR) && BinaryData::namedResourceListSize > 0)
        {
            // Find the first _wav resource and load it
            for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            {
                juce::String resName(BinaryData::namedResourceList[i]);
                if (resName.endsWith("_wav"))
                {
                    int irSize = 0;
                    const char* irData = BinaryData::getNamedResource(resName.toRawUTF8(), irSize);
                    juce::String displayName = resName.dropLastCharacters(4).replaceCharacter('_', ' ');
                    irLoader.loadIR(irData, irSize, displayName);
                    break;
                }
            }
        }
    }
}

void GuitarAmpAudioProcessor::releaseResources() {}

bool GuitarAmpAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Output must be mono or stereo
    const auto& out = layouts.getMainOutputChannelSet();
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

    const int numOut = getTotalNumOutputChannels();

    // Zero all channels above 0 — we process mono through ch0
    // (the user selects the exact input channel via the JUCE audio settings panel)
    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Input trim — first in chain, scales the incoming signal level
    {
        const float trimGain = juce::Decibels::decibelsToGain(
            apvts.getRawParameterValue("inputTrim")->load());
        buffer.applyGain(0, 0, buffer.getNumSamples(), trimGain);
    }

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

    // Pre-amp stage: EQ + compression before distortion
    {
        const bool bypassEq   = apvts.getRawParameterValue("bypassPreEq")->load() > 0.5f;
        const bool bypassComp = apvts.getRawParameterValue("bypassPreComp")->load() > 0.5f;
        preAmpStage.update(
            bypassEq ? 0.0f : apvts.getRawParameterValue("preEqLow")->load(),
            bypassEq ? 0.0f : apvts.getRawParameterValue("preEqMid")->load(),
            bypassEq ? 0.0f : apvts.getRawParameterValue("preEqHigh")->load(),
            apvts.getRawParameterValue("preEqLowFreq")->load(),
            apvts.getRawParameterValue("preEqMidFreq")->load(),
            apvts.getRawParameterValue("preEqHighFreq")->load(),
            apvts.getRawParameterValue("preCompThresh")->load(),
            bypassComp ? 1.0f : apvts.getRawParameterValue("preCompRatio")->load(),
            apvts.getRawParameterValue("preCompAttack")->load(),
            apvts.getRawParameterValue("preCompRelease")->load(),
            bypassComp ? 0.0f : apvts.getRawParameterValue("preCompMakeup")->load(),
            bypassComp ? 0.0f : apvts.getRawParameterValue("preCompBlend")->load());
        preAmpStage.process(buffer);
    }

    // Neural amp processing
    if (apvts.getRawParameterValue("bypassAmp")->load() < 0.5f)
    {
        neuralAmp.update(
            apvts.getRawParameterValue("inputGain")->load(),
            apvts.getRawParameterValue("masterVolume")->load());
        neuralAmp.process(buffer);
    }

    // Post-amp stage: EQ + compression to glue and shape after distortion
    {
        const bool bypassEq   = apvts.getRawParameterValue("bypassPostEq")->load() > 0.5f;
        const bool bypassComp = apvts.getRawParameterValue("bypassPostComp")->load() > 0.5f;
        postAmpStage.update(
            bypassEq ? 0.0f : apvts.getRawParameterValue("postEqLow")->load(),
            bypassEq ? 0.0f : apvts.getRawParameterValue("postEqMid")->load(),
            bypassEq ? 0.0f : apvts.getRawParameterValue("postEqHigh")->load(),
            apvts.getRawParameterValue("postEqLowFreq")->load(),
            apvts.getRawParameterValue("postEqMidFreq")->load(),
            apvts.getRawParameterValue("postEqHighFreq")->load(),
            apvts.getRawParameterValue("postCompThresh")->load(),
            bypassComp ? 1.0f : apvts.getRawParameterValue("postCompRatio")->load(),
            apvts.getRawParameterValue("postCompAttack")->load(),
            apvts.getRawParameterValue("postCompRelease")->load(),
            bypassComp ? 0.0f : apvts.getRawParameterValue("postCompMakeup")->load(),
            bypassComp ? 0.0f : apvts.getRawParameterValue("postCompBlend")->load());
        postAmpStage.process(buffer);
    }

    // IR / Cabinet convolution
    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);
    irLoader.process(buffer);

    // Copy processed ch0 to ch1 for stereo output
    if (numOut >= 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    // Reverb (CloudSeed) — after IR, before MF EQ
    if (reverb && apvts.getRawParameterValue("bypassReverb")->load() < 0.5f)
    {
        float mix   = apvts.getRawParameterValue("reverbMix")->load() / 100.0f;
        float decay = apvts.getRawParameterValue("reverbDecay")->load() / 100.0f;
        float size  = apvts.getRawParameterValue("reverbSize")->load() / 100.0f;

        reverb->SetParameter(Cloudseed::Parameter::LateLineDecay, decay);
        reverb->SetParameter(Cloudseed::Parameter::LateLineSize, size);

        const int numSamples = buffer.getNumSamples();
        float* chL = buffer.getWritePointer(0);
        float* chR = (numOut >= 2) ? buffer.getWritePointer(1) : chL;

        // Save dry signal for mix
        juce::AudioBuffer<float> dryBuf(2, numSamples);
        dryBuf.copyFrom(0, 0, buffer, 0, 0, numSamples);
        if (numOut >= 2)
            dryBuf.copyFrom(1, 0, buffer, 1, 0, numSamples);

        reverb->Process(chL, chR, chL, chR, numSamples);

        // Dry/wet blend
        const float* dryL = dryBuf.getReadPointer(0);
        const float* dryR = dryBuf.getReadPointer(numOut >= 2 ? 1 : 0);
        for (int i = 0; i < numSamples; ++i)
        {
            chL[i] = dryL[i] * (1.0f - mix) + chL[i] * mix;
            if (numOut >= 2)
                chR[i] = dryR[i] * (1.0f - mix) + chR[i] * mix;
        }
    }

    // Post-IR 8-band EQ
    static const char* eqGainIds[EQProcessor::kNumBands] = {
        "eq1Gain","eq2Gain","eq3Gain","eq4Gain",
        "eq5Gain","eq6Gain","eq7Gain","eq8Gain"
    };
    static const char* eqFreqIds[EQProcessor::kNumBands] = {
        "eq1Freq","eq2Freq","eq3Freq","eq4Freq",
        "eq5Freq","eq6Freq","eq7Freq","eq8Freq"
    };
    if (apvts.getRawParameterValue("bypassMfEq")->load() < 0.5f)
    {
        float eqGains[EQProcessor::kNumBands];
        float eqFreqs[EQProcessor::kNumBands];
        for (int b = 0; b < EQProcessor::kNumBands; ++b)
        {
            eqGains[b] = apvts.getRawParameterValue(eqGainIds[b])->load();
            eqFreqs[b] = apvts.getRawParameterValue(eqFreqIds[b])->load();
        }
        eqProcessor.update(eqGains, eqFreqs);
        eqProcessor.process(buffer);
    }

    // Output volume — final gain at end of chain
    {
        const float outGain = juce::Decibels::decibelsToGain(
            apvts.getRawParameterValue("outputVolume")->load());
        buffer.applyGain(outGain);
    }

    // Mute — silence output while keeping tuner active
    if (apvts.getRawParameterValue("muteEnabled")->load() > 0.5f)
        buffer.clear();
}

//==============================================================================
void GuitarAmpAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    // Remove stale children embedded by previous replaceState() calls.
    // Loop handles multiple duplicates from prior buggy save cycles.
    for (auto c = state.getChildWithName("KnobRanges"); c.isValid();
         c = state.getChildWithName("KnobRanges"))
        state.removeChild(c, nullptr);
    for (auto c = state.getChildWithName("SectionImages"); c.isValid();
         c = state.getChildWithName("SectionImages"))
        state.removeChild(c, nullptr);
    for (auto c = state.getChildWithName("SectionColours"); c.isValid();
         c = state.getChildWithName("SectionColours"))
        state.removeChild(c, nullptr);

    state.setProperty("irFilePath",    irLoader.getFilePath(),       nullptr);
    state.setProperty("bundledIRName", irLoader.getFilePath().isEmpty()
                                           ? irLoader.getFileName() : "",  nullptr);
    state.setProperty("ampModelPath",  neuralAmp.getModelFilePath(), nullptr);

    juce::ValueTree rangesTree("KnobRanges");
    for (auto& [id, kr] : knobRanges.ranges)
    {
        juce::ValueTree e("Range");
        e.setProperty("id",  id,     nullptr);
        e.setProperty("min", kr.min, nullptr);
        e.setProperty("max", kr.max, nullptr);
        rangesTree.addChild(e, -1, nullptr);
    }
    state.addChild(rangesTree, -1, nullptr);

    if (sectionImagesTree.isValid() && sectionImagesTree.getNumChildren() > 0)
        state.addChild(sectionImagesTree.createCopy(), -1, nullptr);

    juce::ValueTree coloursTree("SectionColours");
    for (int i = 0; i < SectionColourSet::kNumSections; ++i)
    {
        juce::ValueTree e("Colour");
        e.setProperty("idx", i, nullptr);
        e.setProperty("hex", sectionColours.colours[i].toDisplayString(true), nullptr);
        coloursTree.addChild(e, -1, nullptr);
    }
    state.addChild(coloursTree, -1, nullptr);

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
        {
            irLoader.loadIR(juce::File(irPath));
        }
        else
        {
            // Try to restore a bundled IR by name
            juce::String bundledName = apvts.state.getProperty("bundledIRName", "");
            if (bundledName.isNotEmpty())
            {
                // BinaryData resource name = displayName (with spaces) converted to underscores + "_wav"
                juce::String resName = bundledName.replaceCharacter(' ', '_') + "_wav";
                int irSize = 0;
                const char* irData = BinaryData::getNamedResource(resName.toRawUTF8(), irSize);
                if (irData != nullptr)
                    irLoader.loadIR(irData, irSize, bundledName);
            }
        }

        juce::String ampModelPath = apvts.state.getProperty("ampModelPath", "");
        if (ampModelPath.isNotEmpty())
            neuralAmp.loadModel(juce::File(ampModelPath));

        auto rt = newState.getChildWithName("KnobRanges");
        if (rt.isValid())
        {
            for (int i = 0; i < rt.getNumChildren(); ++i)
            {
                auto e = rt.getChild(i);
                juce::String id = e.getProperty("id").toString();
                float mn = (float)e.getProperty("min");
                float mx = (float)e.getProperty("max");
                if (knobRanges.ranges.count(id))
                    knobRanges.ranges[id] = { mn, mx, knobRanges.ranges[id].skew };
            }
            juce::MessageManager::callAsync([this]
            {
                if (auto* ed = dynamic_cast<GuitarAmpAudioProcessorEditor*>(getActiveEditor()))
                    ed->rebuildAllAttachments();
            });
        }

        auto imagesTree = newState.getChildWithName("SectionImages");
        sectionImagesTree = imagesTree.isValid() ? imagesTree.createCopy()
                                                 : juce::ValueTree("SectionImages");
        juce::MessageManager::callAsync([this]
        {
            if (auto* ed = dynamic_cast<GuitarAmpAudioProcessorEditor*>(getActiveEditor()))
                ed->loadSectionImagesFromTree();
        });

        auto ct = newState.getChildWithName("SectionColours");
        if (ct.isValid())
        {
            for (int i = 0; i < ct.getNumChildren(); ++i)
            {
                auto e = ct.getChild(i);
                int idx = (int)e.getProperty("idx");
                juce::String hex = e.getProperty("hex").toString();
                if (idx >= 0 && idx < SectionColourSet::kNumSections && hex.isNotEmpty())
                    sectionColours.colours[idx] = juce::Colour::fromString(hex);
            }
            juce::MessageManager::callAsync([this]
            {
                if (auto* ed = dynamic_cast<GuitarAmpAudioProcessorEditor*>(getActiveEditor()))
                    ed->applySectionColours();
            });
        }
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
