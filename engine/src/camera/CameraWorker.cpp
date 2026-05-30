#include "camera/CameraWorker.h"

#include <chrono>
#include <utility>

#include <opencv2/imgproc.hpp>

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


void CameraWorker::setFrameCallback(FrameCallback callback)
{
	onFrame = std::move(callback);
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

			// Wait 8 seconds then retry
			for (int i = 0; i < 80 && running; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			continue;
		}

		auto lastEmit = std::chrono::steady_clock::now();

		while (running)
		{
			cv::Mat frame;

			if (!camera.readFrame(frame))
			{
				if (onError) onError(cameraIndex, "Lost connection: " + config.name);
				camera.close();
				break;
			}

			const auto now = std::chrono::steady_clock::now();
			if (now - lastEmit >= std::chrono::milliseconds(33))
			{
				QImage image = convertMatToQImage(frame);
				if (!image.isNull() && onFrame)
					onFrame(cameraIndex, image);
				lastEmit = now;
			}
		}
	}

	camera.close();
	running = false;
}


QImage CameraWorker::convertMatToQImage(const cv::Mat& frame) const
{
	if (frame.empty())
		return {};

	cv::Mat rgbFrame;
	switch (frame.channels()) {
		case 4:  cv::cvtColor(frame, rgbFrame, cv::COLOR_BGRA2RGB); break;
		case 1:  cv::cvtColor(frame, rgbFrame, cv::COLOR_GRAY2RGB); break;
		default: cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);  break;
	}

	QImage image(
		rgbFrame.data,
		rgbFrame.cols,
		rgbFrame.rows,
		static_cast<int>(rgbFrame.step),
		QImage::Format_RGB888
	);

	return image.copy();
}
