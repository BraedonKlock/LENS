#include "camera/CameraCapture.h"

#include <utility>

CameraCapture::CameraCapture(CameraConfig config) : config(std::move(config))
{
}

bool CameraCapture::open()
{
	try {
		if (!capture.open(config.url.toStdString()))
			return false;
		capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
		return true;
	} catch (const cv::Exception&) {
		return false;
	} catch (const std::exception&) {
		return false;
	}
}

bool CameraCapture::readFrame(cv::Mat& frame)
{
	if (!capture.isOpened()) return false;

	try {
		const bool success = capture.read(frame);
		return success && !frame.empty();
	} catch (const cv::Exception&) {
		return false;
	}
}

void CameraCapture::close()
{
	if (capture.isOpened()) capture.release();
}
