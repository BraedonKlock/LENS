#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QPushButton>

class SetupDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SetupDialog(QWidget* parent = nullptr);

private slots:
	void onSubmit();

private:
	QLineEdit*             m_serverId;
	QLineEdit*             m_email;
	QLineEdit*             m_password;
	QLineEdit*             m_storeName;
	QLineEdit*             m_storeNumber;
	QPushButton*           m_submitBtn;
	QLabel*                m_statusLabel;
	QNetworkAccessManager* m_nam;

	void setStatus(const QString& msg, bool isError = false);
	void setLoading(bool loading, const QString& msg = "Creating account...");

	void doRegister();
	void doEngineSetup(const QString& apiKey);
};
