#ifndef CAMERACONFIG_H
#define CAMERACONFIG_H

#include <QString>

struct CameraConfig
{
	QString name;
	QString url;
	QString password;
	QString location;
	bool enabled = true;
};
#endif
