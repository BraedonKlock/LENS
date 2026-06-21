#ifndef INCIDENTHANDLER_H
#define INCIDENTHANDLER_H

#include <opencv2/opencv.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

class IncidentHandler
{
public:
	IncidentHandler(const std::string& backendUrl, const std::string& apiKey);

	void trigger(const std::vector<cv::Mat>& frames, double fps,
	             int cameraId, const std::string& incidentType);

	static void triggerShared(std::shared_ptr<IncidentHandler> self,
	                          const std::vector<cv::Mat>& frames, double fps,
	                          int cameraId, const std::string& incidentType);

	bool isUploading() const { return m_uploading.load(); }

private:
	std::string          m_backendUrl;
	std::string          m_apiKey;
	std::atomic<bool>    m_uploading{false};

	void process(std::vector<cv::Mat> frames, double fps,
	             int cameraId, std::string incidentType);

	std::string recordClip(const std::vector<cv::Mat>& frames, double fps,
	                       const std::string& timestamp);

	bool getPresignedUrl(int cameraId, const std::string& timestamp,
	                     std::string& outUploadUrl, std::string& outClipPath);

	bool uploadClip(const std::string& filePath, const std::string& uploadUrl);

	bool reportIncident(int cameraId, const std::string& timestamp,
	                    const std::string& clipPath, const std::string& incidentType);
};

#endif
