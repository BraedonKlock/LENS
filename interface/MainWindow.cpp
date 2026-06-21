#include "MainWindow.h"
#include "AddCameraDialog.h"
#include "CameraCard.h"
#include "SetupDialog.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
	setWindowTitle("LENS");
	setMinimumSize(820, 560);
	resize(1080, 680);

	m_api = new ApiClient(this);
	connect(m_api, &ApiClient::camerasLoaded,   this, &MainWindow::onCamerasLoaded);
	connect(m_api, &ApiClient::requestFailed,       this, &MainWindow::onEngineError);
	connect(m_api, &ApiClient::cameraAdded,         this, &MainWindow::refresh);
	connect(m_api, &ApiClient::cameraDeleted,       this, &MainWindow::refresh);

	setupUi();

	m_refreshTimer = new QTimer(this);
	m_refreshTimer->setInterval(5000);
	connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::refresh);
	m_refreshTimer->start();

	refresh();
}

void MainWindow::setupUi()
{
	auto* central = new QWidget(this);
	setCentralWidget(central);
	auto* rootLayout = new QVBoxLayout(central);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSpacing(0);

	// ── Header ────────────────────────────────────────────────────────────
	auto* header = new QWidget(central);
	header->setFixedHeight(64);
	header->setStyleSheet("background-color: #040810; border-bottom: 1px solid #0c1628;");
	auto* headerLayout = new QHBoxLayout(header);
	headerLayout->setContentsMargins(24, 0, 24, 0);
	headerLayout->setSpacing(14);

	auto* logoLabel = new QLabel(header);
	QPixmap logo(":/logo/LENS_LOGO.jpg");
	logoLabel->setPixmap(logo.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	auto* titleLabel = new QLabel("LENS", header);
	titleLabel->setStyleSheet(
		"font-size: 20px; font-weight: 700; color: #c8d6ef;"
		"letter-spacing: 3px; background: transparent;");

	auto* divider = new QFrame(header);
	divider->setFrameShape(QFrame::VLine);
	divider->setFixedHeight(24);
	divider->setStyleSheet("color: #0d1e35;");

	m_engineStatus = new QLabel("● Connecting...", header);
	m_engineStatus->setStyleSheet("font-size: 12px; color: #3a5a7a; background: transparent;");

	auto* createAccountBtn = new QPushButton("Create Account", header);
	createAccountBtn->setFixedHeight(32);
	createAccountBtn->setStyleSheet(
		"QPushButton { background: #2563eb; color: white; border: none;"
		" border-radius: 7px; font-size: 12px; font-weight: 600; padding: 0 16px; }"
		"QPushButton:hover { background: #3b82f6; }"
		"QPushButton:pressed { background: #1d4ed8; }");
	connect(createAccountBtn, &QPushButton::clicked, this, [this]() {
		SetupDialog dlg(this);
		if (dlg.exec() == QDialog::Accepted)
			refresh();
	});

	headerLayout->addWidget(logoLabel);
	headerLayout->addWidget(titleLabel);
	headerLayout->addWidget(divider);
	headerLayout->addWidget(m_engineStatus);
	headerLayout->addStretch();
	headerLayout->addWidget(createAccountBtn);

	rootLayout->addWidget(header);

	// ── Content ───────────────────────────────────────────────────────────
	auto* content = new QWidget(central);
	content->setStyleSheet("background-color: #07090f;");
	auto* contentLayout = new QVBoxLayout(content);
	contentLayout->setContentsMargins(32, 28, 32, 28);
	contentLayout->setSpacing(0);

	auto* titleRow    = new QHBoxLayout();
	auto* sectionTitle = new QLabel("Cameras", content);
	sectionTitle->setStyleSheet(
		"font-size: 22px; font-weight: 700; color: #c8d6ef; background: transparent;");

	auto* addBtn = new QPushButton("+ Add Camera", content);
	addBtn->setFixedHeight(38);
	addBtn->setStyleSheet(
		"QPushButton { background: #2563eb; color: white; border: none;"
		" border-radius: 8px; font-size: 13px; font-weight: 600; padding: 0 20px; }"
		"QPushButton:hover { background: #3b82f6; }"
		"QPushButton:pressed { background: #1d4ed8; }");

	titleRow->addWidget(sectionTitle);
	titleRow->addStretch();
	titleRow->addWidget(addBtn);
	contentLayout->addLayout(titleRow);
	contentLayout->addSpacing(6);

	auto* subtitleLabel = new QLabel("Manage the cameras your engine monitors.", content);
	subtitleLabel->setStyleSheet("font-size: 12px; color: #2a4060; background: transparent;");
	contentLayout->addWidget(subtitleLabel);
	contentLayout->addSpacing(24);

	auto* scrollArea = new QScrollArea(content);
	scrollArea->setWidgetResizable(true);
	scrollArea->setFrameShape(QFrame::NoFrame);
	scrollArea->setStyleSheet(
		"QScrollArea { background: transparent; border: none; }"
		"QScrollBar:vertical { background: transparent; width: 6px; }"
		"QScrollBar::handle:vertical { background: #1a2d4a; border-radius: 3px; min-height: 30px; }"
		"QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");

	m_cardsContainer = new QWidget(scrollArea);
	m_cardsContainer->setStyleSheet("background: transparent;");
	m_cardsLayout = new QVBoxLayout(m_cardsContainer);
	m_cardsLayout->setContentsMargins(0, 0, 8, 0);
	m_cardsLayout->setSpacing(10);

	m_emptyLabel = new QLabel(
		"No cameras configured.\nClick \"+ Add Camera\" to get started.", m_cardsContainer);
	m_emptyLabel->setAlignment(Qt::AlignCenter);
	m_emptyLabel->setStyleSheet(
		"font-size: 14px; color: #1e3248; background: transparent;");
	m_emptyLabel->setMinimumHeight(200);
	m_cardsLayout->addWidget(m_emptyLabel);
	m_cardsLayout->addStretch();
	m_emptyLabel->hide();

	scrollArea->setWidget(m_cardsContainer);
	contentLayout->addWidget(scrollArea);
	rootLayout->addWidget(content);

	connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAddCamera);
}

void MainWindow::refresh()
{
	m_api->fetchCameras();
}

void MainWindow::clearCards()
{
	for (int i = m_cardsLayout->count() - 1; i >= 0; --i) {
		auto* item = m_cardsLayout->itemAt(i);
		if (item->widget() && item->widget() != m_emptyLabel) {
			m_cardsLayout->takeAt(i);
			item->widget()->deleteLater();
			delete item;
		}
	}
}

void MainWindow::onCamerasLoaded(const QList<CameraInfo>& cameras)
{
	setEngineOnline(true);
	clearCards();

	if (cameras.isEmpty()) {
		m_emptyLabel->setText("No cameras configured.\nClick \"+ Add Camera\" to get started.");
		m_emptyLabel->show();
		return;
	}

	m_emptyLabel->hide();

	for (const auto& cam : cameras) {
		auto* card = new CameraCard(cam.id, cam.name, cam.url, cam.location, cam.connected, m_cardsContainer);
		connect(card, &CameraCard::deleteRequested, m_api, &ApiClient::deleteCamera);
		m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
	}
}

void MainWindow::onEngineError(const QString&)
{
	setEngineOnline(false);
	clearCards();
	m_emptyLabel->setText("Engine offline.\nMake sure the LENS engine is running.");
	m_emptyLabel->show();
}

void MainWindow::setEngineOnline(bool online)
{
	m_engineOnline = online;
	if (online) {
		m_engineStatus->setText("● Engine Connected");
		m_engineStatus->setStyleSheet("font-size: 12px; color: #22c55e; background: transparent;");
	} else {
		m_engineStatus->setText("● Engine Offline");
		m_engineStatus->setStyleSheet("font-size: 12px; color: #ef4444; background: transparent;");
	}
}

void MainWindow::onAddCamera()
{
	AddCameraDialog dialog(this);
	if (dialog.exec() != QDialog::Accepted) return;

	if (dialog.name().isEmpty() || dialog.url().isEmpty()) {
		QMessageBox::warning(this, "Missing Fields", "Camera name and stream URL are required.");
		return;
	}

	m_api->addCamera(dialog.name(), dialog.url(), dialog.password(), dialog.location());
}
