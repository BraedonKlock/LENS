#ifndef ENGINE_H
#define ENGINE_H

#include "HttpServer.h"

class Engine
{
private:
	HttpServer e_httpServer;
public:
	void start();
	void stop();
};

#endif
