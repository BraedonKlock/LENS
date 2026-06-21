#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;

class AddCameraDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddCameraDialog(QWidget* parent = nullptr);

	QString name()     const;
	QString url()      const;
	QString password() const;
	QString location() const;

private:
	QLineEdit* m_name;
	QLineEdit* m_url;
	QLineEdit* m_password;
	QLineEdit* m_location;
};
