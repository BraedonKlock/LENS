#ifndef CAMERACAPTURE_H
#define CAMERACAPTURE_H

#include <opencv2/opencv.hpp>

class CameraCapture
{
	private:
	CameraConfig config;
	cv::VideoCapture capture;

	public:
	explicit CameraCapture(CameraConfig config);

	bool open();
	bool readFrame(cv::Mat& frame);
	void close();
};

#endif
