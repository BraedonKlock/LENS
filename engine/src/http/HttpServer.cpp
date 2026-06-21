#include "HttpServer.h"

static constexpr const char* HOST = "127.0.0.1";
static constexpr int PORT = 7373;

HttpServer::httpServer()
{
	registerRoutes();
}

void HttpServer::registerRoutes()
{
	server.post("/cameras", [this], const httplib::Request& req, httplib::Response& res)
	{

	};
}

void HttpServer::start()
{
	s_thread = std::thread([this]() 
	{
		server.listen(HOST, PORT);
	});
}

void HttpServer::stop()
{
	s_server.stop();
	if (s_thread.joinable()) s_thread.join();
}
