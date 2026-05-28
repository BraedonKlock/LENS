#ifndef CAMERACONFIG_H
#define CAMERACONFIG_H

#include <QString>

struct CameraConfig
{
	int id = -1;
	QString name;
	QString url;
	QString password;
	QString location;
};
#endif
