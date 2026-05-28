#pragma once

#include <QMainWindow>
#include <QPushButton>
#include "ui_MainWindow.h"

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
    void onAddCameraClicked();

private:
    Ui::MainWindow ui;

    void switchToPage(int index, const QString &title, const QString &subtitle);
    void setNavActive(QPushButton *active);
    void clearCameraForm();
};
