#include "Engine.h"

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>

static std::atomic<bool> running = true;

static void handleSignal(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT,  handleSignal);
	std::signal(SIGTERM, handleSignal);

	std::cout << "LENS engine starting...\n";

	Engine engine;
	engine.start();

  	while (running) std::this_thread::sleep_for(std::chrono::milliseconds(100));

  	engine.stop();

	std::cout << "LENS engine shutting down.\n";
	return 0;
}
