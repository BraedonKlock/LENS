#ifndef ENGINE_H
#define ENGINE_H

#include "HttpServer.h"
#include "CameraManager.h"
#include "CameraStore.h"

class Engine
{
private:
	CameraStore e_cameraStore;
	CameraManager e_cameraManager;
	HttpServer e_httpServer;
public:
	Engine();
	void start();
	void stop();
};

#endif
