#include "camera/CameraWorker.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <utility>

#include <opencv2/opencv.hpp>

#ifdef LENS_INFERENCE_ENABLED
static constexpr int INFERENCE_INTERVAL = 5;
static constexpr int BUFFER_SECONDS     = 5;
static constexpr int COOLDOWN_SECONDS   = 30;

static std::string modelPath()
{
	try {
		auto exeDir = std::filesystem::canonical("/proc/self/exe").parent_path();
		return (exeDir.parent_path() / "models/model.onnx").string();
	} catch (...) {
		return "models/model.onnx";
	}
}
#endif

CameraWorker::CameraWorker(int cameraIndex, CameraConfig config,
                           std::string backendUrl, std::string apiKey)
	: cameraIndex(cameraIndex)
	, config(std::move(config))
	, camera(this->config)
	, m_backendUrl(std::move(backendUrl))
	, m_apiKey(std::move(apiKey))
#ifdef LENS_INFERENCE_ENABLED
	, m_inferencer(modelPath())
	, m_incidentHandler(m_backendUrl, m_apiKey)
#endif
{
}

CameraWorker::~CameraWorker()
{
	stop();
}

void CameraWorker::start()
{
	if (running) return;
	running = true;
	workerThread = std::thread(&CameraWorker::run, this);
}

void CameraWorker::stop()
{
	running = false;
	if (workerThread.joinable()) workerThread.join();
	camera.close();
}

void CameraWorker::setErrorCallback(ErrorCallback callback)
{
	onError = std::move(callback);
}

#ifdef LENS_INFERENCE_ENABLED
bool CameraWorker::isCooldown() const
{
	auto elapsed = std::chrono::steady_clock::now() - m_lastIncident;
	return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() < COOLDOWN_SECONDS;
}
#endif

void CameraWorker::run()
{
#ifdef LENS_INFERENCE_ENABLED
	if (!m_inferencer.load())
		std::cerr << "Inferencer: model not loaded for camera " << config.name << "\n";
#endif

	while (running)
	{
		if (!camera.open())
		{
			m_connected = false;
			if (onError) onError(cameraIndex, "Failed to open camera: " + config.name);

			for (int i = 0; i < 80 && running; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			continue;
		}

		m_connected = true;

#ifdef LENS_INFERENCE_ENABLED
		double fps = camera.getFps();
		m_bufferMaxSize = static_cast<size_t>(fps * BUFFER_SECONDS);
#endif

		while (running)
		{
			cv::Mat frame;

			if (!camera.readFrame(frame))
			{
				m_connected = false;
				if (onError) onError(cameraIndex, "Lost connection: " + config.name);
				camera.close();
				break;
			}

#ifdef LENS_INFERENCE_ENABLED
			m_frameBuffer.push_back(frame.clone());
			if (m_frameBuffer.size() > m_bufferMaxSize)
				m_frameBuffer.pop_front();

			if (++m_frameCount % INFERENCE_INTERVAL == 0 && m_frameBuffer.size() >= 16)
			{
				std::vector<cv::Mat> sample(m_frameBuffer.begin(), m_frameBuffer.end());
				if (m_inferencer.classify(sample) && !isCooldown())
				{
					m_lastIncident = std::chrono::steady_clock::now();
					std::vector<cv::Mat> clip(m_frameBuffer.begin(), m_frameBuffer.end());
					m_incidentHandler.trigger(clip, fps, config.id, "concealment_detected");
				}
			}
#endif
		}
	}

	camera.close();
	running = false;
}
