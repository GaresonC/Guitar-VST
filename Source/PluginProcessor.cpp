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
        juce::NormalisableRange<float>(-20.0f, 80.0f), 20.0f,
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
    preAmpStage.update(
        apvts.getRawParameterValue("preEqLow")->load(),
        apvts.getRawParameterValue("preEqMid")->load(),
        apvts.getRawParameterValue("preEqHigh")->load(),
        apvts.getRawParameterValue("preEqLowFreq")->load(),
        apvts.getRawParameterValue("preEqMidFreq")->load(),
        apvts.getRawParameterValue("preEqHighFreq")->load(),
        apvts.getRawParameterValue("preCompThresh")->load(),
        apvts.getRawParameterValue("preCompRatio")->load(),
        apvts.getRawParameterValue("preCompAttack")->load(),
        apvts.getRawParameterValue("preCompRelease")->load(),
        apvts.getRawParameterValue("preCompMakeup")->load(),
        apvts.getRawParameterValue("preCompBlend")->load());
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
        apvts.getRawParameterValue("postEqLowFreq")->load(),
        apvts.getRawParameterValue("postEqMidFreq")->load(),
        apvts.getRawParameterValue("postEqHighFreq")->load(),
        apvts.getRawParameterValue("postCompThresh")->load(),
        apvts.getRawParameterValue("postCompRatio")->load(),
        apvts.getRawParameterValue("postCompAttack")->load(),
        apvts.getRawParameterValue("postCompRelease")->load(),
        apvts.getRawParameterValue("postCompMakeup")->load(),
        apvts.getRawParameterValue("postCompBlend")->load());
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
