#include "Engine.h"

Engine::Engine()
{
}

void Engine::start()
{
	e_httpServer.start();
}

void Engine::stop()
{
	e_httpServer.stop();
}
