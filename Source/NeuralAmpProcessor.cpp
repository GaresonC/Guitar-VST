#include "NeuralAmpProcessor.h"
#include "json.hpp"

//==============================================================================
// Helpers for GuitarML JSON format loading

static std::vector<std::vector<float>> transpose2D (const std::vector<std::vector<float>>& in)
{
    if (in.empty() || in[0].empty()) return {};
    const int rows = (int)in.size();
    const int cols = (int)in[0].size();
    std::vector<std::vector<float>> out (cols, std::vector<float> (rows));
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
            out[c][r] = in[r][c];
    return out;
}

// Loads a GuitarML-format JSON (model_data + state_dict) into an RTNeural Model.
// Architecture is always: LSTM(input_size, hidden_size) -> Dense(hidden_size, output_size)
static std::unique_ptr<RTNeural::Model<float>> loadGuitarMLModel (const nlohmann::json& j)
{
    auto& md = j.at ("model_data");
    auto& sd = j.at ("state_dict");

    const int inputSize  = md.at ("input_size").get<int>();
    const int hiddenSize = md.at ("hidden_size").get<int>();
    const int outputSize = md.at ("output_size").get<int>();

    // --- LSTM weights ---
    // PyTorch stores weight_ih as [4*H, I] — RTNeural expects [I][4*H] (transposed)
    auto wIH = sd.at ("rec.weight_ih_l0").get<std::vector<std::vector<float>>>();
    auto wHH = sd.at ("rec.weight_hh_l0").get<std::vector<std::vector<float>>>();
    auto bIH = sd.at ("rec.bias_ih_l0").get<std::vector<float>>();
    auto bHH = sd.at ("rec.bias_hh_l0").get<std::vector<float>>();

    // RTNeural expects the combined (ih + hh) bias
    for (int i = 0; i < (int)bHH.size(); ++i)
        bHH[i] += bIH[i];

    auto* lstm = new RTNeural::LSTMLayer<float> (inputSize, hiddenSize);
    lstm->setWVals (transpose2D (wIH));  // [4H][I] -> [I][4H]
    lstm->setUVals (transpose2D (wHH));  // [4H][H] -> [H][4H]
    lstm->setBVals (bHH);               // [4H] combined bias

    // --- Dense weights ---
    auto wLin = sd.at ("lin.weight").get<std::vector<std::vector<float>>>();
    auto bLin = sd.at ("lin.bias").get<std::vector<float>>();

    auto* dense = new RTNeural::Dense<float> (hiddenSize, outputSize);
    dense->setWeights (wLin);       // [out][hidden]
    dense->setBias (bLin.data());  // const T* required

    auto model = std::make_unique<RTNeural::Model<float>> (inputSize);
    model->addLayer (lstm);
    model->addLayer (dense);

    return model;
}

//==============================================================================
void NeuralAmpProcessor::prepare (double /*sampleRate*/, int /*samplesPerBlock*/)
{
    juce::SpinLock::ScopedLockType lock (modelLock);
    if (model)
        model->reset();
}

void NeuralAmpProcessor::update (float inputGainDb, float outputLevelDb)
{
    inputGainLin  = juce::Decibels::decibelsToGain (inputGainDb);
    outputGainLin = juce::Decibels::decibelsToGain (outputLevelDb);
}

void NeuralAmpProcessor::process (juce::AudioBuffer<float>& buffer)
{
    const float inGain  = inputGainLin;
    const float outGain = outputGainLin;
    float* ch0 = buffer.getWritePointer (0);
    const int   n      = buffer.getNumSamples();

    juce::SpinLock::ScopedTryLockType lock (modelLock);
    if (!lock.isLocked() || !model)
    {
        // No model loaded: pass through with output gain applied
        buffer.applyGain (0, 0, n, outGain);
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        float input = ch0[i] * inGain;
        ch0[i] = model->forward (&input) * outGain;
    }
}

bool NeuralAmpProcessor::loadModel (const juce::File& jsonFile)
{
    if (!jsonFile.existsAsFile())
        return false;

    std::ifstream stream (jsonFile.getFullPathName().toStdString());
    if (!stream.is_open())
        return false;

    nlohmann::json j;
    try { j = nlohmann::json::parse (stream); }
    catch (...) { return false; }

    std::unique_ptr<RTNeural::Model<float>> newModel;
    try
    {
        if (j.contains ("model_data") && j.contains ("state_dict"))
            newModel = loadGuitarMLModel (j);           // GuitarML format
        else
        {
            stream.clear();
            stream.seekg (0);
            newModel = RTNeural::json_parser::parseJson<float> (stream);  // native RTNeural format
        }
    }
    catch (...) { return false; }

    if (!newModel)
        return false;

    newModel->reset();

    {
        juce::SpinLock::ScopedLockType lock (modelLock);
        model = std::move (newModel);
        currentFile = jsonFile;
        modelReady.store (true);
    }

    return true;
}
