#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "httplib.h"
#include "CameraManager.h"
#include "CameraStore.h"
#include "IncidentHandler.h"
#include <memory>
#include <thread>

#ifdef LENS_INFERENCE_ENABLED
#include "Inferencer.h"
#endif

class HttpServer
{
private:
	httplib::Server  s_server;
	CameraManager&   s_cameraManager;
	CameraStore&     s_cameraStore;
	std::shared_ptr<IncidentHandler> s_testHandler;
	void registerRoutes();
	std::thread s_thread;

public:
	HttpServer(CameraManager& manager, CameraStore& store);

	HttpServer(const HttpServer&) = delete;
	HttpServer& operator=(const HttpServer&) = delete;

	void start();
	void stop();
};

#endif
