#include "MainWindow.h"

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
    connect(ui.addCameraButton,    &QPushButton::clicked, this, &MainWindow::onAddCameraClicked);
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
    const QString location = ui.cameraLocationInput->text().trimmed();
    const bool    enabled  = ui.cameraEnabledCheckbox->isChecked();

    if (name.isEmpty() || url.isEmpty())
        return; // TODO: show validation feedback

    int row = ui.cameraConfigTable->rowCount();
    ui.cameraConfigTable->insertRow(row);
    ui.cameraConfigTable->setItem(row, 0, new QTableWidgetItem(name));
    ui.cameraConfigTable->setItem(row, 1, new QTableWidgetItem(url));
    ui.cameraConfigTable->setItem(row, 2, new QTableWidgetItem(location));
    ui.cameraConfigTable->setItem(row, 3, new QTableWidgetItem(enabled ? "Enabled" : "Disabled"));

    clearCameraForm();
}

void MainWindow::onCancelCameraClicked() {
    clearCameraForm();
}

void MainWindow::onAddCameraClicked() {
    clearCameraForm();
    ui.cameraNameInput->setFocus();
}

void MainWindow::clearCameraForm() {
    ui.cameraNameInput->clear();
    ui.cameraUrlInput->clear();
    ui.cameraUsernameInput->clear();
    ui.cameraPasswordInput->clear();
    ui.cameraLocationInput->clear();
    ui.cameraEnabledCheckbox->setChecked(true);
}
