#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

ApiClient::ApiClient(QObject* parent) : QObject(parent)
{
	m_nam = new QNetworkAccessManager(this);
}

void ApiClient::checkSetup()
{
	QNetworkRequest req(QUrl(QString("%1/config/status").arg(BASE)));
	auto* reply = m_nam->get(req);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}
		const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
		emit setupStatusReceived(obj["configured"].toBool(true));
	});
}

void ApiClient::fetchCameras()
{
	QNetworkRequest req(QUrl(QString("%1/cameras").arg(BASE)));
	auto* reply = m_nam->get(req);

	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}

		QList<CameraInfo> cameras;
		auto arr = QJsonDocument::fromJson(reply->readAll()).array();
		for (const auto& val : arr) {
			auto obj = val.toObject();
			cameras.append({ obj["id"].toInt(), obj["name"].toString(),
			                 obj["url"].toString(), obj["location"].toString(),
			                 obj["connected"].toBool(false) });
		}
		emit camerasLoaded(cameras);
	});
}

void ApiClient::addCamera(const QString& name, const QString& url,
                           const QString& password, const QString& location)
{
	QNetworkRequest req(QUrl(QString("%1/cameras").arg(BASE)));
	req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	QJsonObject body;
	body["name"]     = name;
	body["url"]      = url;
	body["password"] = password;
	body["location"] = location;

	auto* reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}
		emit cameraAdded();
	});
}

void ApiClient::deleteCamera(int id)
{
	QNetworkRequest req(QUrl(QString("%1/cameras/%2").arg(BASE).arg(id)));
	auto* reply = m_nam->deleteResource(req);
	connect(reply, &QNetworkReply::finished, this, [this, reply]() {
		reply->deleteLater();
		if (reply->error() != QNetworkReply::NoError) {
			emit requestFailed(reply->errorString());
			return;
		}
		emit cameraDeleted();
	});
}
