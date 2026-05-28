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

void start()
{
	if (running) return;

	running = true;

	workerThread = std::thread(&CameraWorker::run, this);
}

void CamerWorker::stop()
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
	if (!camera.open())
	{
		if (onError)
		{
			onError(cameraIndex, "Falied to open camera: " + config.name);
		}
		
		running = false;
		return;
	}

	while (running)
	{
		cv::Mat frame;

		if (!camera.readFrame(frame))
		{
			if (onError)
			{
				onError(cameraIndex, "Failed to read frame from camera: " + config.name);
			}
			
			break;
		}
		
		QImage image = convertMatToQImage(frame);

		if (!image.isNull && onFrame)
		{
			onFrame(cameraIndex, image);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}

	camera.close;
	running = false;
}


QImage CameraWorker::convertMatToQImage(const cv::Mat& frame) const
{
	if (frame.empty()) 
	{
        	return {};
    	}

    	cv::Mat rgbFrame;
    	cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    	QImage image(
        	rgbFrame.data,
        	rgbFrame.cols,
        	rgbFrame.rows,
        	static_cast<int>(rgbFrame.step),
        	QImage::Format_RGB888
    	);

    	return image.copy();
}
