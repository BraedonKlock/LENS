#ifndef CAMERAWORKER_H
#define CAMERAWORKER_H

#include "CameraCapture.h"
#include "CameraConfig.h"

#include <QImage>
#include <QString>

#include <opencv2/opencv.hpp>

#include <thread>
#include <atomic>
#include <functional>

class CameraWorker
{
	public:
	CameraWorker(int cameraIndex, CameraConfig config);
	CameraWorker(const CameraWorker&) = delete;

	CameraWorker& operator=(const CameraWorker&) = delete;
	
	void start();
	void stop();
	
	using FrameCallback = std::function<void(int cameraIndex, const QImage& image)>;
	using ErrorCallback = std::function<void(int cameraIndex, const QString& message)>;

	void setFrameCallback(FrameCallback callback);
	void setErrorCallback(ErrorCallback callback);

	~CameraWorker();

	private:
	int cameraIndex;
	
	FrameCallback onFrame;
	ErrorCallback onError;

	CameraConfig config;
	CameraCapture camera;
	
	std::thread workerThread;
	
	std::atomic<bool> running = false;

	void run();
	
	QImage convertMatToQImage(const cv::Mat& frame) const;
};
#endif
