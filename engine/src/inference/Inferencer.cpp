#include "Inferencer.h"

#include <cmath>
#include <iostream>
#include <numeric>

// ImageNet normalization constants (standard VideoMAE preprocessing)
static constexpr float MEAN[3] = {0.485f, 0.456f, 0.406f};
static constexpr float STD[3]  = {0.229f, 0.224f, 0.225f};

Inferencer::Inferencer(const std::string& modelPath)
	: m_modelPath(modelPath)
	, m_env(ORT_LOGGING_LEVEL_WARNING, "LENS")
{
	m_sessionOptions.SetIntraOpNumThreads(2);
	m_sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
}

Inferencer::~Inferencer() = default;

bool Inferencer::load()
{
	try {
		m_session = std::make_unique<Ort::Session>(m_env, m_modelPath.c_str(), m_sessionOptions);
		return true;
	} catch (const Ort::Exception& e) {
		std::cerr << "[Inferencer] load failed: " << e.what() << "\n";
		return false;
	}
}

std::vector<float> Inferencer::prepareInput(const std::vector<cv::Mat>& frames)
{
	// Sample NUM_FRAMES evenly from whatever we were given
	std::vector<int> indices(NUM_FRAMES);
	const int total = static_cast<int>(frames.size());
	for (int i = 0; i < NUM_FRAMES; ++i)
		indices[i] = std::min(i * total / NUM_FRAMES, total - 1);

	// Output tensor: [1, 16, 3, 224, 224] in CHW order per frame
	std::vector<float> data;
	data.reserve(NUM_FRAMES * 3 * INPUT_SIZE * INPUT_SIZE);

	for (int fi : indices) {
		cv::Mat resized, rgb;
		cv::resize(frames[fi], resized, cv::Size(INPUT_SIZE, INPUT_SIZE));
		cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);
		rgb.convertTo(rgb, CV_32F, 1.0 / 255.0);

		// Split into channels and normalize, then write in CHW order
		std::vector<cv::Mat> channels(3);
		cv::split(rgb, channels);
		for (int c = 0; c < 3; ++c) {
			channels[c] = (channels[c] - MEAN[c]) / STD[c];
			data.insert(data.end(), (float*)channels[c].datastart, (float*)channels[c].dataend);
		}
	}

	return data;
}

bool Inferencer::classify(const std::vector<cv::Mat>& frames, float threshold)
{
	if (!m_session || frames.empty()) return false;

	auto inputData = prepareInput(frames);

	std::vector<int64_t> inputShape = {1, NUM_FRAMES, 3, INPUT_SIZE, INPUT_SIZE};
	auto memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
	Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
		memInfo, inputData.data(), inputData.size(), inputShape.data(), inputShape.size());

	Ort::AllocatorWithDefaultOptions allocator;
	auto inName  = m_session->GetInputNameAllocated(0, allocator);
	auto outName = m_session->GetOutputNameAllocated(0, allocator);
	const char* inNames[]  = {inName.get()};
	const char* outNames[] = {outName.get()};

	auto outputs = m_session->Run(Ort::RunOptions{nullptr}, inNames, &inputTensor, 1, outNames, 1);

	float* logits = outputs[0].GetTensorMutableData<float>();
	// logits[0] = normal, logits[1] = suspicious
	// Apply softmax to get probabilities
	float maxL  = std::max(logits[0], logits[1]);
	float e0    = std::exp(logits[0] - maxL);
	float e1    = std::exp(logits[1] - maxL);
	float prob  = e1 / (e0 + e1);

	std::cerr << "[Inferencer] suspicious prob=" << prob
	          << " (threshold=" << threshold << ")\n";

	return prob >= threshold;
}
