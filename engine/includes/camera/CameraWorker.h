#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include "CameraCapture.h"
#include "CameraConfig.h"

#include <thread>
#include <atomic>
#include <functional>
#include <string>

class CameraWorker
{
public:
	using ErrorCallback = std::function<void(int cameraIndex, const std::string& message)>;

	CameraWorker(int cameraIndex, CameraConfig config);
	CameraWorker(const CameraWorker&) = delete;
	CameraWorker& operator=(const CameraWorker&) = delete;
	~CameraWorker();

	void start();
	void stop();
	void setErrorCallback(ErrorCallback callback);

private:
	int cameraIndex;
	CameraConfig config;
	CameraCapture camera;

	ErrorCallback onError;

	std::thread workerThread;
	std::atomic<bool> running = false;

	void run();
};
#endif
