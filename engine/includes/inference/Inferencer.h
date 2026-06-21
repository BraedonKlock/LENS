#ifndef INFERENCER_H
#define INFERENCER_H

#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

class Inferencer
{
public:
	explicit Inferencer(const std::string& modelPath);
	~Inferencer();

	Inferencer(const Inferencer&)            = delete;
	Inferencer& operator=(const Inferencer&) = delete;

	bool load();

	// Returns true if the clip (any number of frames) is classified as suspicious.
	// Evenly samples 16 frames from the input, runs VideoMAE, checks class-1 logit.
	bool classify(const std::vector<cv::Mat>& frames, float threshold = 0.5f);

private:
	static constexpr int   NUM_FRAMES  = 16;
	static constexpr int   INPUT_SIZE  = 224;

	std::string                    m_modelPath;
	Ort::Env                       m_env;
	Ort::SessionOptions            m_sessionOptions;
	std::unique_ptr<Ort::Session>  m_session;

	std::vector<float> prepareInput(const std::vector<cv::Mat>& frames);
};

#endif
