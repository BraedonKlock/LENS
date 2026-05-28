#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include <QImage>
#include <QString>

#include <functional>
#include <memory>
#include <vector>

#include "CameraConfig.h"
#include "CameraWorker.h"

class CameraManager
{
	public:
    	using FrameCallback = std::function<void(int cameraIndex, const QImage& image)>;
    	using ErrorCallback = std::function<void(int cameraIndex, const QString& message)>;

    	CameraManager() = default;
    	~CameraManager();

    	CameraManager(const CameraManager&) = delete;
    	CameraManager& operator=(const CameraManager&) = delete;

    	void addCamera(const CameraConfig& config);
    	void startAll();
    	void stopAll();

    	void setFrameCallback(FrameCallback callback);
    	void setErrorCallback(ErrorCallback callback);

	private:
    	std::vector<std::unique_ptr<CameraWorker>> workers;

    	FrameCallback onFrame;
    	ErrorCallback onError;
};
#endif
