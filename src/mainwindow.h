#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPixmap>
#include <QSharedPointer>
#include <QList>
#include "stitcher.h"
#include "voxel_container.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(StitcherImpl* _stitcher, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileLoadButton_clicked();
    void on_sliceSpinBox_valueChanged(int slice);
    void on_slicePlaneBox_currentIndexChanged(int plane);
    void on_scansListrowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);

private:
    void updateDisplay(int plane, int slice);
    void updateStitch();

    Ui::MainWindow *ui;

    const int partialScansCount = 1; // Temporary fixed
    QList<QSharedPointer<VoxelContainer>> partialScans;
    QSharedPointer<VoxelContainer> stitchedScan;
    QPixmap currSlice;
    StitcherImpl* stitcher;
};

#endif // MAINWINDOW_H
