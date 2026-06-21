#pragma once

#include <QList>
#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

struct CameraInfo
{
	int     id        = -1;
	QString name;
	QString url;
	QString location;
	bool    connected = false;
};

class ApiClient : public QObject
{
	Q_OBJECT

public:
	explicit ApiClient(QObject* parent = nullptr);

	void checkSetup();
	void fetchCameras();
	void addCamera(const QString& name, const QString& url,
	               const QString& password, const QString& location);
	void deleteCamera(int id);

signals:
	void setupStatusReceived(bool configured);
	void camerasLoaded(const QList<CameraInfo>& cameras);
	void cameraAdded();
	void cameraDeleted();
	void requestFailed(const QString& message);

private:
	QNetworkAccessManager* m_nam;
	static constexpr const char* BASE = "http://127.0.0.1:7373";
};
