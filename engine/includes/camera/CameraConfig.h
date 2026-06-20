#ifndef CAMERACONFIG_H
#define CAMERACONFIG_H

#include <string>

struct CameraConfig
{
	int id = -1;
	std::string name;
	std::string url;
	std::string password;
	std::string location;
};
#endif
