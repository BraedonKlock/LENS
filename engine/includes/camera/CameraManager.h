#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <functional>
#include <memory>
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

	void addCamera(const CameraConfig& config);
	void startAll();
	void stopAll();

	void setErrorCallback(ErrorCallback callback);

private:
	std::vector<std::unique_ptr<CameraWorker>> workers;
	ErrorCallback onError;
};
#endif
