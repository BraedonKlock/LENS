#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include "camera/CameraConfig.h"

#include <opencv2/opencv.hpp>

class CameraCapture
{
	public:
	explicit CameraCapture(CameraConfig config);

	bool open();
	bool readFrame(cv::Mat& frame);
	void close();

	private:
	bool isFileSource() const;

	CameraConfig config;
	cv::VideoCapture capture;
};

#endif
