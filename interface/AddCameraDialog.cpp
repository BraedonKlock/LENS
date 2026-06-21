#include "AddCameraDialog.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

AddCameraDialog::AddCameraDialog(QWidget* parent) : QDialog(parent)
{
	setWindowTitle("Add Camera");
	setFixedSize(460, 420);
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(32, 28, 32, 28);
	root->setSpacing(0);

	// Title
	auto* title = new QLabel("Add Camera", this);
	title->setStyleSheet("font-size: 20px; font-weight: 700; color: #d4e4f7; margin-bottom: 4px;");
	root->addWidget(title);

	auto* subtitle = new QLabel("Configure a new camera stream for LENS to monitor.", this);
	subtitle->setStyleSheet("font-size: 12px; color: #3a5a7a; margin-bottom: 20px;");
	subtitle->setWordWrap(true);
	root->addWidget(subtitle);

	root->addSpacing(8);

	// Field builder
	auto addField = [&](const QString& label, QLineEdit*& out,
	                    const QString& placeholder, bool isPassword = false)
	{
		auto* lbl = new QLabel(label, this);
		lbl->setStyleSheet(
			"font-size: 11px; font-weight: 700; color: #3a5a7a;"
			"text-transform: uppercase; letter-spacing: 0.8px; margin-bottom: 4px;");
		root->addWidget(lbl);

		out = new QLineEdit(this);
		out->setPlaceholderText(placeholder);
		out->setFixedHeight(38);
		if (isPassword) out->setEchoMode(QLineEdit::Password);
		root->addWidget(out);
		root->addSpacing(12);
	};

	addField("Camera Name", m_name,     "e.g. Front Door");
	addField("Stream URL",  m_url,      "rtsp://192.168.1.100/stream");
	addField("Password",    m_password, "Camera password (optional)", true);
	addField("Location",    m_location, "e.g. Entrance");

	root->addStretch();

	// Buttons
	auto* btnRow = new QHBoxLayout();
	btnRow->setSpacing(10);
	btnRow->addStretch();

	auto* cancel = new QPushButton("Cancel", this);
	cancel->setFixedSize(90, 36);
	cancel->setStyleSheet(
		"QPushButton { background: #0d1525; color: #5a7a9e; border: 1px solid #1a2e4a;"
		" border-radius: 7px; font-size: 13px; }"
		"QPushButton:hover { background: #111d33; color: #8aaed4; }");

	auto* add = new QPushButton("Add Camera", this);
	add->setFixedSize(110, 36);
	add->setDefault(true);
	add->setStyleSheet(
		"QPushButton { background: #2563eb; color: white; border: none;"
		" border-radius: 7px; font-size: 13px; font-weight: 600; }"
		"QPushButton:hover { background: #3b82f6; }"
		"QPushButton:pressed { background: #1d4ed8; }");

	btnRow->addWidget(cancel);
	btnRow->addWidget(add);
	root->addLayout(btnRow);

	connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
	connect(add,    &QPushButton::clicked, this, &QDialog::accept);
}

QString AddCameraDialog::name()     const { return m_name->text().trimmed(); }
QString AddCameraDialog::url()      const { return m_url->text().trimmed(); }
QString AddCameraDialog::password() const { return m_password->text(); }
QString AddCameraDialog::location() const { return m_location->text().trimmed(); }
