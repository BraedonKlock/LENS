#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include "CameraCapture.h"
#include "CameraConfig.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <string>
#include <thread>

#ifdef LENS_INFERENCE_ENABLED
#include "Inferencer.h"
#include "IncidentHandler.h"
#endif

class CameraWorker
{
public:
	using ErrorCallback = std::function<void(int cameraIndex, const std::string& message)>;

	CameraWorker(int cameraIndex, CameraConfig config,
	             std::string backendUrl = {}, std::string apiKey = {});
	CameraWorker(const CameraWorker&) = delete;
	CameraWorker& operator=(const CameraWorker&) = delete;
	~CameraWorker();

	void start();
	void stop();
	void setErrorCallback(ErrorCallback callback);
	int  cameraId()    const { return config.id; }
	bool isConnected() const { return m_connected.load(); }

private:
	int           cameraIndex;
	CameraConfig  config;
	CameraCapture camera;
	std::string   m_backendUrl;
	std::string   m_apiKey;

#ifdef LENS_INFERENCE_ENABLED
	Inferencer      m_inferencer;
	IncidentHandler m_incidentHandler;
	std::deque<cv::Mat>                   m_frameBuffer;
	size_t                                m_bufferMaxSize = 150;
	int                                   m_frameCount    = 0;
	std::chrono::steady_clock::time_point m_lastIncident;
#endif

	ErrorCallback     onError;
	std::thread       workerThread;
	std::atomic<bool> running   = false;
	std::atomic<bool> m_connected = false;

	void run();
#ifdef LENS_INFERENCE_ENABLED
	bool isCooldown() const;
#endif
};
#endif
