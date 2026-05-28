#include "camera/CameraCapture.h"

#include <utility>

CameraCapture::CameraCapture(CameraConfig config) : config(std::move(config))
{
}	

bool CameraCapture::open()
{
	return capture.open(config.url.toStdString());
}

bool CameraCapture::readFrame(cv::Mat& frame)
{
	if (!capture.isOpened()) return false;

	const bool success = capture.read(frame);

	return success && !frame.empty();
}

void CameraCapture::close()
{
	if (capture.isOpened()) capture.release();
}
