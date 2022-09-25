#include <QErrorMessage>
#include <QFileDialog>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::updateDisplay(int plane, int slice) {
    switch (plane) {
        case 0:
            ui->display->setPixmap(partialScans[0]->getXSlice(slice));
            break;

        case 1:
            ui->display->setPixmap(partialScans[0]->getYSlice(slice));
            break;

        case 2:
            ui->display->setPixmap(partialScans[0]->getZSlice(slice));
            break;
    }
}

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

//    ui->sliceSpinBox->setMaximum();

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
}


void MainWindow::on_sliceSpinBox_valueChanged(int slice) {
    updateDisplay(ui->slicePlaneBox->currentIndex(), slice);
}


void MainWindow::on_slicePlaneBox_currentIndexChanged(int plane) {
    updateDisplay(plane, ui->sliceSpinBox->value());
}
