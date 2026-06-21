#include "CameraCard.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

CameraCard::CameraCard(int id, const QString& name, const QString& url,
                        const QString& location, bool connected, QWidget* parent)
	: QFrame(parent), m_id(id)
{
	setObjectName("cameraCard");
	setFixedHeight(92);
	setStyleSheet(
		"QFrame#cameraCard {"
		"  background-color: #0b1120;"
		"  border: 1px solid #111d33;"
		"  border-radius: 10px;"
		"}"
		"QFrame#cameraCard:hover {"
		"  border-color: #1e3a6a;"
		"  background-color: #0d1428;"
		"}");

	auto* mainLayout = new QHBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 20, 0);
	mainLayout->setSpacing(0);

	// Left accent bar
	auto* accent = new QFrame(this);
	accent->setFixedWidth(4);
	accent->setStyleSheet(
		"background-color: #2563eb;"
		"border-top-left-radius: 10px;"
		"border-bottom-left-radius: 10px;"
		"border-radius: 0px;"  // reset then set individual corners below
	);
	// QSS doesn't support individual border-radius per side cleanly, so we use a margin trick
	accent->setStyleSheet("background-color: #2563eb; border-radius: 0px;");
	mainLayout->addWidget(accent);
	mainLayout->addSpacing(20);

	// Info section
	auto* infoLayout = new QVBoxLayout();
	infoLayout->setSpacing(3);
	infoLayout->setContentsMargins(0, 14, 0, 14);

	auto* nameLabel = new QLabel(name, this);
	nameLabel->setStyleSheet("font-size: 14px; font-weight: 700; color: #d4e4f7; background: transparent;");

	QString displayUrl = url.length() > 55 ? url.left(52) + "..." : url;
	auto* urlLabel = new QLabel(displayUrl, this);
	urlLabel->setStyleSheet("font-size: 12px; color: #304d6a; background: transparent;");
	urlLabel->setToolTip(url);

	auto* locLabel = new QLabel(location.isEmpty() ? "No location set" : location, this);
	locLabel->setStyleSheet(QString("font-size: 11px; color: %1; background: transparent;")
		.arg(location.isEmpty() ? "#1e3048" : "#3a5a7a"));

	infoLayout->addWidget(nameLabel);
	infoLayout->addWidget(urlLabel);
	infoLayout->addWidget(locLabel);

	mainLayout->addLayout(infoLayout);
	mainLayout->addStretch();

	// Right section: status badge + delete button
	auto* rightLayout = new QVBoxLayout();
	rightLayout->setContentsMargins(0, 14, 0, 14);
	rightLayout->setSpacing(0);

	// Status badge — colours driven by live connection state
	const QString dotColor    = connected ? "#22c55e" : "#f59e0b";
	const QString textColor   = connected ? "#22c55e" : "#f59e0b";
	const QString bgColor     = connected ? "#071a0f"  : "#1a1200";
	const QString borderColor = connected ? "#0f3020"  : "#3a2800";
	const QString statusText  = connected ? "Connected" : "Connecting...";

	auto* badge = new QWidget(this);
	badge->setFixedHeight(22);
	badge->setStyleSheet(
		QString("background-color: %1; border: 1px solid %2; border-radius: 11px;")
			.arg(bgColor, borderColor));
	auto* badgeLayout = new QHBoxLayout(badge);
	badgeLayout->setContentsMargins(8, 0, 10, 0);
	badgeLayout->setSpacing(5);

	auto* dot = new QFrame(badge);
	dot->setFixedSize(7, 7);
	dot->setStyleSheet(QString("background-color: %1; border-radius: 3px;").arg(dotColor));

	auto* statusLabel = new QLabel(statusText, badge);
	statusLabel->setStyleSheet(
		QString("font-size: 11px; color: %1; font-weight: 600; background: transparent;")
			.arg(textColor));

	badgeLayout->addWidget(dot);
	badgeLayout->addWidget(statusLabel);
	badge->adjustSize();

	rightLayout->addWidget(badge, 0, Qt::AlignRight);
	rightLayout->addStretch();

	// Delete button
	auto* deleteBtn = new QPushButton("Delete", this);
	deleteBtn->setFixedSize(66, 28);
	deleteBtn->setStyleSheet(
		"QPushButton { background: transparent; color: #ef4444; border: 1px solid #2a1212;"
		" border-radius: 6px; font-size: 11px; font-weight: 600; }"
		"QPushButton:hover { background: #1a0808; border-color: #ef4444; }"
		"QPushButton:pressed { background: #2a0a0a; }");

	rightLayout->addWidget(deleteBtn, 0, Qt::AlignRight);
	mainLayout->addLayout(rightLayout);

	connect(deleteBtn, &QPushButton::clicked, this, [this, name]() {
		auto* box = new QMessageBox(this);
		box->setWindowTitle("Delete Camera");
		box->setText(QString("Remove <b>%1</b> from LENS?").arg(name));
		box->setInformativeText("The engine will stop monitoring this camera.");
		box->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		box->setDefaultButton(QMessageBox::Cancel);
		box->setStyleSheet("QMessageBox { background-color: #0a0e1a; color: #c8d6ef; }");
		if (box->exec() == QMessageBox::Yes)
			emit deleteRequested(m_id);
	});
}
