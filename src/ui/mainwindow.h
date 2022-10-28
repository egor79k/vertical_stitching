#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QGraphicsPixmapItem>
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

    void on_removeScanButton_clicked();

private:
    void updateSliceBounds(int plane);
    void updateDisplay(int plane, int slice);
    void updateStitch();
    void wheelEvent(QWheelEvent* event);

    Ui::MainWindow *ui;

    QGraphicsScene displayScene;
    QGraphicsPixmapItem currSliceItem;

    QList<std::shared_ptr<VoxelContainer>> partialScans;
    std::shared_ptr<VoxelContainer> stitchedScan;
    StitcherImpl* stitcher;
};

#endif // MAINWINDOW_H
