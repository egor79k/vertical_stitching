#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QSharedPointer>
#include <QVector>
#include "voxel_container.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileLoadButton_clicked();
    void on_sliceSpinBox_valueChanged(int slice);
    void on_slicePlaneBox_currentIndexChanged(int plane);

private:
    void updateDisplay(int plane, int slice);

    Ui::MainWindow *ui;

    const int partialScansCount = 1; // Temporary fixed
    QVector<QSharedPointer<VoxelContainer>> partialScans;

    QPixmap currSlice;
};

#endif // MAINWINDOW_H
