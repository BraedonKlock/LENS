#include "MainWindow.h"

#include <QMetaObject>
#include <QPixmap>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);

    // Set live cameras as the default active page
    setNavActive(ui.liveCamerasButton);

    // Nav buttons
    connect(ui.liveCamerasButton, &QPushButton::clicked, this, &MainWindow::onLiveCamerasClicked);
    connect(ui.incidentsButton,   &QPushButton::clicked, this, &MainWindow::onIncidentsClicked);
    connect(ui.settingsButton,    &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    // Settings — camera form
    connect(ui.saveCameraButton,   &QPushButton::clicked, this, &MainWindow::onSaveCameraClicked);
    connect(ui.cancelCameraButton, &QPushButton::clicked, this, &MainWindow::onCancelCameraClicked);

    // Reset all 4 camera slots to unconfigured state
    for (int i = 0; i < 4; ++i) {
        if (auto* lbl = titleLabelAt(i))   lbl->setText(QString("Camera %1").arg(i + 1));
        if (auto* lbl = frameLabelAt(i))   lbl->setText("No camera configured");
        if (auto* lbl = statusLabelAt(i))  lbl->setVisible(false);
    }

    // CameraManager callbacks — marshal from worker threads to GUI thread
    cameraManager.setFrameCallback([this](int index, const QImage& image) {
        QMetaObject::invokeMethod(this, [this, index, image]() {
            updateCameraFrame(index, image);
        }, Qt::QueuedConnection);
    });

    cameraManager.setErrorCallback([this](int index, const QString& message) {
        QMetaObject::invokeMethod(this, [this, index, message]() {
            updateCameraError(index, message);
        }, Qt::QueuedConnection);
    });
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainWindow::onLiveCamerasClicked() {
    switchToPage(0, "Live Cameras",
                 "Monitor camera feeds and suspicious activity alerts in real time.");
    setNavActive(ui.liveCamerasButton);
}

void MainWindow::onIncidentsClicked() {
    switchToPage(1, "Incidents",
                 "Review and manage detected suspicious activity.");
    setNavActive(ui.incidentsButton);
}

void MainWindow::onSettingsClicked() {
    switchToPage(2, "Settings",
                 "Configure cameras, detection zones, and system preferences.");
    setNavActive(ui.settingsButton);
}

void MainWindow::switchToPage(int index, const QString &title, const QString &subtitle) {
    ui.pagesStackedWidget->setCurrentIndex(index);
    ui.pageTitleLabel->setText(title);
    ui.pageSubtitleLabel->setText(subtitle);
}

void MainWindow::setNavActive(QPushButton *active) {
    for (QPushButton *btn : {ui.liveCamerasButton, ui.incidentsButton, ui.settingsButton}) {
        btn->setProperty("navActive", btn == active);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

// ── Camera form ───────────────────────────────────────────────────────────────

void MainWindow::onSaveCameraClicked() {
    const QString name     = ui.cameraNameInput->text().trimmed();
    const QString url      = ui.cameraUrlInput->text().trimmed();
    const QString password = ui.cameraPasswordInput->text().trimmed();
    const QString location = ui.cameraLocationInput->text().trimmed();
    const bool    enabled  = ui.cameraEnabledCheckbox->isChecked();

    const bool nameValid = !name.isEmpty();
    const bool urlValid  = !url.isEmpty();

    ui.cameraNameInput->setStyleSheet(nameValid ? "" : "border: 1px solid #ef4444;");
    ui.cameraUrlInput->setStyleSheet(urlValid  ? "" : "border: 1px solid #ef4444;");

    if (!nameValid) { ui.cameraNameInput->setFocus(); return; }
    if (!urlValid)  { ui.cameraUrlInput->setFocus();  return; }

    int row = ui.cameraConfigTable->rowCount();
    ui.cameraConfigTable->insertRow(row);
    ui.cameraConfigTable->setItem(row, 0, new QTableWidgetItem(name));
    ui.cameraConfigTable->setItem(row, 1, new QTableWidgetItem(url));
    ui.cameraConfigTable->setItem(row, 2, new QTableWidgetItem(location));
    ui.cameraConfigTable->setItem(row, 3, new QTableWidgetItem(enabled ? "Enabled" : "Disabled"));

    if (enabled && m_cameraCount < 4) {
        const int cardIndex = m_cameraCount++;

        if (auto* lbl = titleLabelAt(cardIndex)) {
            const QString title = location.isEmpty() ? name : name + " \xe2\x80\x94 " + location;
            lbl->setText(title);
        }
        if (auto* status = statusLabelAt(cardIndex)) {
            status->setVisible(true);
            status->setText("● CONNECTING");
        }

        CameraConfig cfg;
        cfg.name     = name;
        cfg.url      = url;
        cfg.password = password;
        cfg.location = location;
        cfg.enabled  = true;

        cameraManager.addCamera(cfg);
        cameraManager.startAll();
    }

    clearCameraForm();
}

void MainWindow::onCancelCameraClicked() {
    clearCameraForm();
}

void MainWindow::clearCameraForm() {
    ui.cameraNameInput->clear();
    ui.cameraUrlInput->clear();
    ui.cameraUsernameInput->clear();
    ui.cameraPasswordInput->clear();
    ui.cameraLocationInput->clear();
    ui.cameraEnabledCheckbox->setChecked(true);
    ui.cameraNameInput->setStyleSheet("");
    ui.cameraUrlInput->setStyleSheet("");
}

// ── Camera card updates (called on GUI thread via QMetaObject::invokeMethod) ──

void MainWindow::updateCameraFrame(int index, const QImage &image) {
    QLabel* frame = frameLabelAt(index);
    if (!frame) return;

    frame->setPixmap(
        QPixmap::fromImage(image).scaled(frame->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)
    );

    if (auto* status = statusLabelAt(index))
        status->setText("● LIVE");
}

void MainWindow::updateCameraError(int index, const QString &message) {
    Q_UNUSED(message)

    if (auto* status = statusLabelAt(index))
        status->setText("● OFFLINE");
    if (auto* frame = frameLabelAt(index))
        frame->setText("Connection lost");
}

// ── Per-index label helpers ────────────────────────────────────────────────────

QLabel* MainWindow::frameLabelAt(int index) const {
    switch (index) {
        case 0: return ui.cameraPlaceholder1;
        case 1: return ui.cameraPlaceholder2;
        case 2: return ui.cameraPlaceholder3;
        case 3: return ui.cameraPlaceholder4;
        default: return nullptr;
    }
}

QLabel* MainWindow::statusLabelAt(int index) const {
    switch (index) {
        case 0: return ui.cameraStatusLabel1;
        case 1: return ui.cameraStatusLabel2;
        case 2: return ui.cameraStatusLabel3;
        case 3: return ui.cameraStatusLabel4;
        default: return nullptr;
    }
}

QLabel* MainWindow::titleLabelAt(int index) const {
    switch (index) {
        case 0: return ui.cameraTitleLabel1;
        case 1: return ui.cameraTitleLabel2;
        case 2: return ui.cameraTitleLabel3;
        case 3: return ui.cameraTitleLabel4;
        default: return nullptr;
    }
}
