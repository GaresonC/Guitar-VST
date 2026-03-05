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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"channel", 1}, "Channel",
        juce::NormalisableRange<float>(0.0f, 2.0f, 1.0f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"irEnabled", 1}, "IR Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"tunerEnabled", 1}, "Tuner Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"noiseGateEnabled", 1}, "Gate Enabled", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseGateThreshold", 1}, "Gate Threshold",
        juce::NormalisableRange<float>(-80.0f, -20.0f), -60.0f));

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

    ampProcessor.prepare(sampleRate, samplesPerBlock);
    tuner.prepare(sampleRate, samplesPerBlock);
    irLoader.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Apply current parameter values on prepare
    ampProcessor.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("bass")->load(),
        apvts.getRawParameterValue("mid")->load(),
        apvts.getRawParameterValue("treble")->load(),
        apvts.getRawParameterValue("presence")->load(),
        apvts.getRawParameterValue("masterVolume")->load(),
        (int)apvts.getRawParameterValue("channel")->load());

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

    // Amp processing
    ampProcessor.update(
        apvts.getRawParameterValue("inputGain")->load(),
        apvts.getRawParameterValue("bass")->load(),
        apvts.getRawParameterValue("mid")->load(),
        apvts.getRawParameterValue("treble")->load(),
        apvts.getRawParameterValue("presence")->load(),
        apvts.getRawParameterValue("masterVolume")->load(),
        (int)apvts.getRawParameterValue("channel")->load());
    ampProcessor.process(buffer);

    // IR / Cabinet convolution
    irLoader.setEnabled(apvts.getRawParameterValue("irEnabled")->load() > 0.5f);
    irLoader.process(buffer);
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
