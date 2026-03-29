#pragma once
#include <JuceHeader.h>
#include <RTNeural/RTNeural.h>
#include <fstream>
#include <memory>

// Real-time neural amp modelling via RTNeural (LSTM + Dense).
// Supports two JSON formats:
//   1. GuitarML (PyTorch export): detected by "model_data" + "state_dict" keys.
//      Weights are transposed from PyTorch [4H,I] layout to RTNeural [I,4H].
//   2. Native RTNeural JSON: parsed directly by RTNeural's built-in loader.
// Thread safety: model loading acquires a SpinLock; process() uses a TryLock
// so the audio thread never blocks — it falls back to pass-through if locked.
class NeuralAmpProcessor
{
public:
    // Call on the audio thread when the DAW prepares the graph.
    void prepare(double sampleRate, int samplesPerBlock);

    // Process channel 0 in-place. If no model is loaded, applies output gain only.
    void process(juce::AudioBuffer<float>& buffer);

    // Load an RTNeural JSON model file. Safe to call from any thread.
    // Returns true on success.
    bool loadModel(const juce::File& jsonFile);

    // Load a model from in-memory JSON data (e.g. BinaryData). Safe to call from any thread.
    bool loadModel(const char* jsonData, int jsonSize);

    bool hasModel() const noexcept { return modelReady.load(); }

    juce::String getModelFileName() const { return currentFile.getFileName(); }
    juce::String getModelFilePath() const { return currentFile.getFullPathName(); }

    // Update gain parameters. Call before process() each block.
    void update(float inputGainDb, float outputLevelDb);

private:
    std::unique_ptr<RTNeural::Model<float>> model;
    juce::SpinLock   modelLock;
    std::atomic<bool> modelReady { false };
    juce::File        currentFile;

    float inputGainLin  = 1.0f;
    float outputGainLin = juce::Decibels::decibelsToGain (-6.0f);
};
