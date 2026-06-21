#pragma once

#include <QFrame>

class CameraCard : public QFrame
{
	Q_OBJECT

public:
	CameraCard(int id, const QString& name, const QString& url,
	           const QString& location, bool connected, QWidget* parent = nullptr);

signals:
	void deleteRequested(int id);

private:
	int m_id;
};
