#pragma once

#include "ApiClient.h"

#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QVBoxLayout>

class QScrollArea;
class QTimer;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);

private slots:
	void onAddCamera();
	void onCamerasLoaded(const QList<CameraInfo>& cameras);
	void onEngineError(const QString& msg);

private:
	ApiClient*   m_api;
	QWidget*     m_cardsContainer;
	QVBoxLayout* m_cardsLayout;
	QLabel*      m_engineStatus;
	QLabel*      m_emptyLabel;
	QTimer*      m_refreshTimer;
	bool         m_engineOnline = false;

	void setupUi();
	void refresh();
	void clearCards();
	void setEngineOnline(bool online);
};
