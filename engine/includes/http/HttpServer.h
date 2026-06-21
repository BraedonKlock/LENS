# ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "httplib.h"
#include <thread>

class HttpServer
{
private:
	httplib::Server server;
	void registerRoutes();
	std::thread s_thread;

public:
	HttpServer();
	
	HttpServer(const HttpServer&) = delete;
	HttpServer operator=(const HttpServer&) = delete;

	void start();
	void stop();
};

#endif
