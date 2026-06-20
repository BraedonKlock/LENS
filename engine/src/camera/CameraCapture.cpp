#include "camera/CameraCapture.h"

#include <utility>

CameraCapture::CameraCapture(CameraConfig config) : config(std::move(config))
{
}

bool CameraCapture::open()
{
	try {
		if (!capture.open(config.url))
			return false;
		capture.set(cv::CAP_PROP_BUFFERSIZE, 1);
		return true;
	} catch (const cv::Exception&) {
		return false;
	} catch (const std::exception&) {
		return false;
	}
}

bool CameraCapture::isFileSource() const
{
	return config.url.rfind("rtsp://", 0) != 0
		&& config.url.rfind("rtmp://", 0) != 0
		&& config.url.rfind("http://", 0) != 0
		&& config.url.rfind("https://", 0) != 0;
}

bool CameraCapture::readFrame(cv::Mat& frame)
{
	if (!capture.isOpened()) return false;

	try {
		bool success = capture.read(frame);

		if (isFileSource() && (!success || frame.empty())) {
			capture.set(cv::CAP_PROP_POS_FRAMES, 0);
			success = capture.read(frame);
		}

		return success && !frame.empty();
	} catch (const cv::Exception&) {
		return false;
	}
}

void CameraCapture::close()
{
	if (capture.isOpened()) capture.release();
}
