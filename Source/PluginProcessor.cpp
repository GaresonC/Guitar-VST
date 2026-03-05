#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        juce::ParameterID{"irEnabled", 1}, "IR Enabled", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"tunerEnabled", 1}, "Tuner Enabled", true));

    return { params.begin(), params.end() };
}

//==============================================================================
void GuitarAmpAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
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
}

void GuitarAmpAudioProcessor::releaseResources() {}

void GuitarAmpAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                            juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any output channels that aren't mapped to an input
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Tuner runs on the dry signal (channel 0)
    if (apvts.getRawParameterValue("tunerEnabled")->load() > 0.5f)
        tuner.process(buffer);

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
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GuitarAmpAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
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
