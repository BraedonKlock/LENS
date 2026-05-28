#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QPushButton>

#include "ui_MainWindow.h"
#include "camera/CameraManager.h"
#include "camera/CameraStore.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onLiveCamerasClicked();
    void onIncidentsClicked();
    void onSettingsClicked();
    void onSaveCameraClicked();
    void onCancelCameraClicked();
    void onDeleteCameraClicked();

private:
    Ui::MainWindow ui;
    CameraManager cameraManager;
    CameraStore m_store;
    int m_cameraCount = 0;

    void switchToPage(int index, const QString &title, const QString &subtitle);
    void setNavActive(QPushButton *active);
    void clearCameraForm();
    void registerCamera(const CameraConfig& config);

    void updateCameraFrame(int index, const QImage &image);
    void updateCameraError(int index, const QString &message);

    QLabel* frameLabelAt(int index) const;
    QLabel* statusLabelAt(int index) const;
    QLabel* titleLabelAt(int index) const;
};
