#include "camera/CameraManager.h"
#include <algorithm>

CameraManager::~CameraManager()
{
	stopAll();
}

bool CameraManager::isConnected(int cameraId) const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& w : workers)
		if (w->cameraId() == cameraId)
			return w->isConnected();
	return false;
}

void CameraManager::setConfig(const std::string& backendUrl, const std::string& apiKey)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_backendUrl = backendUrl;
	m_apiKey     = apiKey;
}

void CameraManager::addCamera(const CameraConfig& config)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	const int cameraIndex = static_cast<int>(workers.size());
	auto worker = std::make_unique<CameraWorker>(cameraIndex, config, m_backendUrl, m_apiKey);
	worker->setErrorCallback([this](int index, const std::string& message) {
		if (onError) onError(index, message);
	});

	if (m_started) worker->start();

	workers.push_back(std::move(worker));
}

void CameraManager::removeCamera(int cameraId)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto it = std::find_if(workers.begin(), workers.end(),
		[cameraId](const std::unique_ptr<CameraWorker>& w) {
			return w->cameraId() == cameraId;
		});

	if (it != workers.end()) {
		(*it)->stop();
		workers.erase(it);
	}
}

void CameraManager::startAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_started = true;
	for (auto& worker : workers)
		worker->start();
}

void CameraManager::stopAll()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& worker : workers)
		worker->stop();
	workers.clear();
	m_started = false;
}

void CameraManager::setErrorCallback(ErrorCallback callback)
{
	onError = std::move(callback);
}
