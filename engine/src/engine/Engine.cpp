#include "Engine.h"
#include <iostream>

Engine::Engine()
	: e_cameraStore("database/lens.db")
	, e_httpServer(e_cameraManager, e_cameraStore)
{
}

void Engine::start()
{
	if (!e_cameraStore.open())
	{
		std::cerr << "Failed to open database\n";
		return;
	}

	const std::string apiKey     = e_cameraStore.getConfig("api_key");
	const std::string backendUrl = e_cameraStore.getConfig("backend_url");

	if (!apiKey.empty() && !backendUrl.empty())
		e_cameraManager.setConfig(backendUrl, apiKey);

	for (const auto& config : e_cameraStore.loadAll())
		e_cameraManager.addCamera(config);

	e_cameraManager.startAll();
	e_httpServer.start();
}

void Engine::stop()
{
	e_cameraManager.stopAll();
	e_httpServer.stop();
}
