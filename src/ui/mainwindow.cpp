#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "tiff_image.h"


MainWindow::MainWindow(AlgoList* stitchAlgos_, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    stitchedScan(std::make_shared<VoxelContainer>()),
    stitchAlgos(stitchAlgos_),
    stitcher(stitchAlgos_->first().first) {
    ui->setupUi(this);

    displayScene.addItem(&currSliceItem);
    ui->graphicsView->setScene(&displayScene);

    for (auto algo : *stitchAlgos) {
        ui->algorithmBox->addItem(algo.second);
    }

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
    TiffImage<uint8_t> img;
    stitchedScan->getSlice<uint8_t>(img, plane, slice, true);
    currSliceItem.setPixmap(QPixmap::fromImage(QImage(img.getData(), img.getWidth(), img.getHeight(), QImage::Format_Grayscale8)));

    // Fit slice into view frame
    ui->graphicsView->fitInView(displayScene.sceneRect(), Qt::KeepAspectRatio);
}


void MainWindow::updateStitch() {
    if (partialScans.size() > 1) {
        stitchedScan = stitcher->stitch(*partialScans[0], *partialScans[1]);

        if (stitchedScan == nullptr) {
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

    if (event->angleDelta().y() < 0) {
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
        QDir::currentPath(),
        "Parameters (*.json);;Images (*.png *.tiff *.jpg);;All files (*.*)");

    if (fileNames.isEmpty()) {
        return;
    }

    partialScans.append(std::make_shared<VoxelContainer>());

    // Convert QStringList to std::vector<std::string>
    std::vector<std::string> fileNamesStd(fileNames.size());

    for (int i = 0; i < fileNames.size(); ++i) {
        fileNamesStd[i] = fileNames[i].toStdString();
    }

    if (fileNames.size() == 1 && fileNames.back().endsWith(".json")) {
        // Try to load from parameters
        if (!partialScans.last()->loadFromJson(fileNamesStd.back())) {
            partialScans.removeLast();
            QMessageBox::information(nullptr, "File error", QString("Unable to read JSON file '%1'").arg(fileNames.back()));
            return;
        }
    }
    else {
        // Try to load from chosen images
        if (!partialScans.last()->loadFromImages(fileNamesStd)) {
            QMessageBox::information(nullptr, "File error", QString("Unable to read image files"));
            partialScans.removeLast();
            return;
        }
    }


    auto scanShape = partialScans.last()->getSize();

    // Add new scan to visible list
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


void MainWindow::on_algorithmBox_currentIndexChanged(int index) {
    stitcher = stitchAlgos->at(index).first;
    updateStitch();
}
