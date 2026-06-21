#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "CameraConfig.h"
#include "CameraWorker.h"

class CameraManager
{
public:
	using ErrorCallback = std::function<void(int cameraIndex, const std::string& message)>;

	CameraManager() = default;
	~CameraManager();

	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	void setConfig(const std::string& backendUrl, const std::string& apiKey);

	void addCamera(const CameraConfig& config);
	bool isConnected(int cameraId) const;
	void removeCamera(int cameraId);
	void startAll();
	void stopAll();

	void setErrorCallback(ErrorCallback callback);

private:
	std::vector<std::unique_ptr<CameraWorker>> workers;
	ErrorCallback onError;
	mutable std::mutex m_mutex;
	bool          m_started    = false;
	std::string   m_backendUrl;
	std::string   m_apiKey;
};
#endif
