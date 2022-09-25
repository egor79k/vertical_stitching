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

    connect(ui->scansList->model(), SIGNAL(rowsMoved(QModelIndex, int, int, QModelIndex, int)), this, SLOT(on_scansListrowsMoved(QModelIndex, int, int, QModelIndex, int)));

//    new QListWidgetItem("111", ui->scansList);
//    new QListWidgetItem("222", ui->scansList);
//    new QListWidgetItem("333", ui->scansList);
//    new QListWidgetItem("444", ui->scansList);
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::updateDisplay(int plane, int slice) {
    switch (plane) {
        case 0:
            ui->display->setPixmap(stitchedScan->getXSlice(slice));
            break;

        case 1:
            ui->display->setPixmap(stitchedScan->getYSlice(slice));
            break;

        case 2:
            ui->display->setPixmap(stitchedScan->getZSlice(slice));
            break;
    }
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
}


//void MainWindow::updateSliceBounds() {
//    auto size = stitchedScan->getSize();
//    ui->sliceSpinBox->setMaximum(size.);
//}

/*
void MainWindow::on_fileLoadButton_clicked() {
    partialScans.resize(partialScansCount);

    for (int i = 0; i < partialScansCount; ++i) {
        // Start open file dialog to get image names
        QStringList fileNames = QFileDialog::getOpenFileNames(this,
            "Open files",
            QDir::currentPath(),
            "Images (*.png *.tiff *.jpg);;All files (*.*)");

        if (fileNames.isEmpty()) {
            return;
        }
        
        // Try to load chosen images
        if (!partialScans[i].loadFromFiles(fileNames)) {
            QErrorMessage errorMessage;
            errorMessage.showMessage("Failed to open image files.");
            errorMessage.exec();
            return;
        }
    }
}
*/

void MainWindow::on_fileLoadButton_clicked() {
    // Start open file dialog to get image names
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
        "Open files",
        QDir::currentPath(),
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
    updateDisplay(plane, ui->sliceSpinBox->value());
}


void MainWindow::on_scansListrowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row) {
    if (start < row) {
        --row;
    }

    partialScans.swap(start, row);

    updateStitch();
}
