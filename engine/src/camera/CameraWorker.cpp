#include "camera/CameraWorker.h"

#include <chrono>
#include <utility>

#include <opencv2/opencv.hpp>

CameraWorker::CameraWorker(int cameraIndex, CameraConfig config)
	: cameraIndex(cameraIndex), config(std::move(config)), camera(this->config)
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

void CameraWorker::run()
{
	while (running)
	{
		if (!camera.open())
		{
			if (onError) onError(cameraIndex, "Failed to open camera: " + config.name);

			for (int i = 0; i < 80 && running; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			continue;
		}

		while (running)
		{
			cv::Mat frame;

			if (!camera.readFrame(frame))
			{
				if (onError) onError(cameraIndex, "Lost connection: " + config.name);
				camera.close();
				break;
			}

			// TODO: pass frame to AI inference
		}
	}

	camera.close();
	running = false;
}
