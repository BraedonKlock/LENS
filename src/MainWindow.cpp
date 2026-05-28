#include "MainWindow.h"

#include <QCoreApplication>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMetaObject>
#include <QPixmap>
#include <QSizePolicy>
#include <QTableWidget>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_store(QCoreApplication::applicationDirPath() + "/data")
{
    ui.setupUi(this);

    setNavActive(ui.liveCamerasButton);

    connect(ui.liveCamerasButton, &QPushButton::clicked, this, &MainWindow::onLiveCamerasClicked);
    connect(ui.incidentsButton,   &QPushButton::clicked, this, &MainWindow::onIncidentsClicked);
    connect(ui.settingsButton,    &QPushButton::clicked, this, &MainWindow::onSettingsClicked);

    connect(ui.saveCameraButton,    &QPushButton::clicked,               this, &MainWindow::onSaveCameraClicked);
    connect(ui.cancelCameraButton,  &QPushButton::clicked,               this, &MainWindow::onCancelCameraClicked);
    connect(ui.deleteCameraButton,  &QPushButton::clicked,               this, &MainWindow::onDeleteCameraClicked);
    connect(ui.cameraConfigTable,   &QTableWidget::itemSelectionChanged, this, [this]() {
        ui.deleteCameraButton->setEnabled(ui.cameraConfigTable->currentRow() >= 0);
    });

    // Make the scroll area background match the page background
    ui.cameraScrollArea->viewport()->setStyleSheet("background-color: #070d1a;");

    // Equal-width columns in the camera grid
    auto* grid = qobject_cast<QGridLayout*>(ui.cameraScrollContents->layout());
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);

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

    m_store.open();
    for (const auto& cfg : m_store.loadAll())
        registerCamera(cfg);
    cameraManager.startAll();
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

    const bool nameValid = !name.isEmpty();
    const bool urlValid  = !url.isEmpty();

    ui.cameraNameInput->setStyleSheet(nameValid ? "" : "border: 1px solid #ef4444;");
    ui.cameraUrlInput->setStyleSheet(urlValid  ? "" : "border: 1px solid #ef4444;");

    if (!nameValid) { ui.cameraNameInput->setFocus(); return; }
    if (!urlValid)  { ui.cameraUrlInput->setFocus();  return; }

    CameraConfig cfg;
    cfg.name     = name;
    cfg.url      = url;
    cfg.password = password;
    cfg.location = location;

    cfg.id = m_store.save(cfg);
    registerCamera(cfg);
    cameraManager.startAll();
    clearCameraForm();
}

void MainWindow::onCancelCameraClicked() {
    clearCameraForm();
}

void MainWindow::onDeleteCameraClicked() {
    const int row = ui.cameraConfigTable->currentRow();
    if (row < 0) return;

    const QString name = ui.cameraConfigTable->item(row, 0)->text();
    const auto answer = QMessageBox::question(
        this,
        "Delete Camera",
        QString("Delete \"%1\"? This cannot be undone.").arg(name),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
    );
    if (answer != QMessageBox::Yes) return;

    const int id = ui.cameraConfigTable->item(row, 0)->data(Qt::UserRole).toInt();
    m_store.remove(id);

    cameraManager.stopAll();
    clearCameraCards();
    ui.cameraConfigTable->setRowCount(0);
    ui.deleteCameraButton->setEnabled(false);

    for (const auto& cfg : m_store.loadAll())
        registerCamera(cfg);
    cameraManager.startAll();
}

void MainWindow::registerCamera(const CameraConfig& cfg) {
    const int row = ui.cameraConfigTable->rowCount();
    ui.cameraConfigTable->insertRow(row);
    auto* nameItem = new QTableWidgetItem(cfg.name);
    nameItem->setData(Qt::UserRole, cfg.id);
    ui.cameraConfigTable->setItem(row, 0, nameItem);
    ui.cameraConfigTable->setItem(row, 1, new QTableWidgetItem(cfg.url));
    ui.cameraConfigTable->setItem(row, 2, new QTableWidgetItem(cfg.location));

    const int cardIndex = static_cast<int>(m_frameLabels.size());
    addCameraCard(cardIndex, cfg);
    cameraManager.addCamera(cfg);
}

void MainWindow::addCameraCard(int cardIndex, const CameraConfig& cfg) {
    auto* frame = new QFrame(ui.cameraScrollContents);
    frame->setStyleSheet(
        "QFrame { background-color: #030711; border: 1px solid #1a2540; border-radius: 12px; }"
    );
    frame->setMinimumHeight(260);

    auto* cardLayout = new QGridLayout(frame);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    auto* placeholder = new QLabel("No feed connected", frame);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    placeholder->setMinimumSize(1, 1);
    placeholder->setStyleSheet(
        "background-color: #030711; color: #1e3a5f; font-size: 13px; font-weight: 500;"
    );
    cardLayout->addWidget(placeholder, 0, 0);

    auto* overlay = new QWidget(frame);
    overlay->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        " stop:0 rgba(3,7,17,0.88), stop:1 rgba(3,7,17,0));"
    );
    auto* overlayLayout = new QHBoxLayout(overlay);
    overlayLayout->setContentsMargins(12, 8, 12, 8);
    overlayLayout->setSpacing(8);

    const QString title = cfg.location.isEmpty()
        ? cfg.name
        : cfg.name + " \xe2\x80\x94 " + cfg.location;
    auto* titleLabel = new QLabel(title, overlay);
    titleLabel->setStyleSheet(
        "color: #e2e8f0; font-size: 13px; font-weight: 600; background-color: transparent;"
    );
    overlayLayout->addWidget(titleLabel);
    overlayLayout->addStretch();

    auto* statusLabel = new QLabel("● CONNECTING", overlay);
    statusLabel->setStyleSheet(
        "background-color: rgba(16, 185, 129, 0.15);"
        " color: #34d399;"
        " border: 1px solid rgba(16, 185, 129, 0.3);"
        " border-radius: 8px;"
        " padding: 3px 10px;"
        " font-size: 11px;"
        " font-weight: 700;"
        " letter-spacing: 0.5px;"
    );
    overlayLayout->addWidget(statusLabel);

    cardLayout->addWidget(overlay, 0, 0, Qt::AlignTop);

    auto* grid = qobject_cast<QGridLayout*>(ui.cameraScrollContents->layout());
    grid->addWidget(frame, cardIndex / 2, cardIndex % 2);

    m_frameLabels.append(placeholder);
    m_titleLabels.append(titleLabel);
    m_statusLabels.append(statusLabel);
}

void MainWindow::clearCameraCards() {
    m_frameLabels.clear();
    m_titleLabels.clear();
    m_statusLabels.clear();

    auto* grid = qobject_cast<QGridLayout*>(ui.cameraScrollContents->layout());
    while (grid->count() > 0) {
        auto* item = grid->takeAt(0);
        if (auto* widget = item->widget())
            widget->deleteLater();
        delete item;
    }
}

void MainWindow::clearCameraForm() {
    ui.cameraNameInput->clear();
    ui.cameraUrlInput->clear();
    ui.cameraUsernameInput->clear();
    ui.cameraPasswordInput->clear();
    ui.cameraLocationInput->clear();
    ui.cameraNameInput->setStyleSheet("");
    ui.cameraUrlInput->setStyleSheet("");
}

// ── Camera card updates (called on GUI thread via QMetaObject::invokeMethod) ──

void MainWindow::updateCameraFrame(int index, const QImage &image) {
    QLabel* frame = frameLabelAt(index);
    if (!frame) return;

    const QSize available = frame->contentsRect().size();
    if (available.isEmpty()) return;

    frame->setPixmap(
        QPixmap::fromImage(image).scaled(available, Qt::KeepAspectRatio, Qt::SmoothTransformation)
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
    return (index >= 0 && index < m_frameLabels.size()) ? m_frameLabels[index] : nullptr;
}

QLabel* MainWindow::statusLabelAt(int index) const {
    return (index >= 0 && index < m_statusLabels.size()) ? m_statusLabels[index] : nullptr;
}

QLabel* MainWindow::titleLabelAt(int index) const {
    return (index >= 0 && index < m_titleLabels.size()) ? m_titleLabels[index] : nullptr;
}
