#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(StitcherImpl* _stitcher, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    stitcher(_stitcher) {
    ui->setupUi(this);

    displayScene.addItem(&currSliceItem);
    ui->graphicsView->setScene(&displayScene);

    connect(ui->scansList->model(),
        SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)),
        this,
        SLOT(on_scansListrowsMoved(QModelIndex, int, int, QModelIndex, int)));
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::updateSliceBounds(int plane) {
    if (stitchedScan->isEmpty()) {
        ui->sliceSpinBox->setMaximum(0);
        return;
    }

    auto size = stitchedScan->getSize();

    switch (plane) {
        case 0:
            ui->sliceSpinBox->setMaximum(size.x - 1);
            break;

        case 1:
            ui->sliceSpinBox->setMaximum(size.y - 1);
            break;

        case 2:
            ui->sliceSpinBox->setMaximum(size.z - 1);
            break;
    }
}


void MainWindow::updateDisplay(int plane, int slice) {
    currSliceItem.setPixmap(stitchedScan->getSlice(plane, slice));

    // Fit slice into view frame
    ui->graphicsView->fitInView(displayScene.sceneRect(), Qt::KeepAspectRatio);
}


void MainWindow::updateStitch() {
    if (partialScans.size() > 1) {
        stitchedScan = stitcher->stitch(*partialScans[0], *partialScans[1]);

        if (stitchedScan.isNull()) {
            return;
        }
    }
    else if (partialScans.size() == 1) {
        stitchedScan = partialScans[0];
    }
    else {
        stitchedScan->clear();
    }

    int plane = ui->slicePlaneBox->currentIndex();
    updateSliceBounds(plane);
    updateDisplay(plane, ui->sliceSpinBox->value());
}


void MainWindow::wheelEvent(QWheelEvent* event) {
    qreal scaleFactor = 0.9;

    if (event->delta() < 0) {
        ui->graphicsView->scale(scaleFactor, scaleFactor);
    }
    else {
        ui->graphicsView->scale(1 / scaleFactor, 1 / scaleFactor);
    }
}


void MainWindow::on_fileLoadButton_clicked() {
    // Start open file dialog to get image names
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
        "Open files",
//        QDir::currentPath(),
        "/home/egor/projects/TomoPhantom/img/Shepp-Logan/"
        "Images (*.png *.tiff *.jpg);;All files (*.*)");

    if (fileNames.isEmpty()) {
        return;
    }

    // Try to load chosen images
    partialScans.append(QSharedPointer<VoxelContainer>::create());

    if (!partialScans.last()->loadFromFiles(fileNames)) {
        QErrorMessage errorMessage;
        errorMessage.showMessage("Failed to load image files.");
        errorMessage.exec();
        partialScans.removeLast();
        return;
    }

    auto scanShape = partialScans.last()->getSize();

    new QListWidgetItem(
        "Scan " +
        QString::number(partialScans.size()) +
        "   " +
        QString::number(scanShape.x) +
        "x" +
        QString::number(scanShape.y) +
        "x" +
        QString::number(scanShape.z),
        ui->scansList);

    updateStitch();
}


void MainWindow::on_sliceSpinBox_valueChanged(int slice) {
    updateDisplay(ui->slicePlaneBox->currentIndex(), slice);
}


void MainWindow::on_slicePlaneBox_currentIndexChanged(int plane) {
    updateSliceBounds(plane);
    updateDisplay(plane, ui->sliceSpinBox->value());
}


void MainWindow::on_scansListrowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row) {
    if (start < row) {
        --row;
    }

    partialScans.move(start, row);

    updateStitch();
}


void MainWindow::on_removeScanButton_clicked()
{
    int currentScanId = ui->scansList->currentRow();

    if (currentScanId >= 0) {
        ui->scansList->takeItem(currentScanId);
        partialScans.removeAt(currentScanId);
        updateStitch();
    }

}
