#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <vector>
#include <QGraphicsScene>
#include <QMainWindow>
#include <QGraphicsPixmapItem>
#include <QList>
#include <QWheelEvent>
#include "stitcher.h"
#include "voxel_container.h"


using AlgoList = QList<QPair<std::shared_ptr<StitcherImpl>, QString>>;


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(AlgoList* stitchAlgos_, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_fileLoadButton_clicked();
    void on_sliceSpinBox_valueChanged(int slice);
    void on_slicePlaneBox_currentIndexChanged(int plane);
    void on_scansListrowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row);
    void on_removeScanButton_clicked();
    void on_algorithmBox_currentIndexChanged(int index);
    // Menu bar
    void on_actionSave_triggered();
    void on_actionSaveSlice_triggered();
    void on_actionExportSlice_triggered();

private:
    void updateSliceBounds(int plane);
    void updateDisplay(int plane, int slice);
    void updateStitch();
    void appendScansList();
    void wheelEvent(QWheelEvent* event);

    Ui::MainWindow *ui;

    QGraphicsScene displayScene;
    QGraphicsPixmapItem currSliceItem;

    std::vector<std::shared_ptr<VoxelContainer>> partialScans;
    std::shared_ptr<VoxelContainer> stitchedScan;

    std::shared_ptr<StitcherImpl> stitcher;
    AlgoList* stitchAlgos;
};

#endif // MAINWINDOW_H
