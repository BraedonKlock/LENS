#include "camera/CameraManager.h"

CameraManager::~CameraManager()
{
    	stopAll();
}

void CameraManager::addCamera(const CameraConfig& config)
{
    	const int cameraIndex = static_cast<int>(workers.size());

    	auto worker = std::make_unique<CameraWorker>(cameraIndex, config);

    	worker->setFrameCallback([this](int index, const QImage& image) {
        	if (onFrame) 
		{
            		onFrame(index, image);
        	}
    	});

    	worker->setErrorCallback([this](int index, const QString& message) {
        	if (onError) 
		{
            		onError(index, message);
        	}
    	});

    	workers.push_back(std::move(worker));
}

void CameraManager::startAll()
{
    	for (auto& worker : workers) 
	{
        	worker->start();
    	}
}

void CameraManager::stopAll()
{
    	for (auto& worker : workers) 
	{
        	worker->stop();
    	}

    	workers.clear();
}

void CameraManager::setFrameCallback(FrameCallback callback)
{
    	onFrame = std::move(callback);
}

void CameraManager::setErrorCallback(ErrorCallback callback)
{
    	onError = std::move(callback);
}
