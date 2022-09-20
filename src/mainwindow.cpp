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


void MainWindow::on_fileLoadButton_clicked() {
    partialScans.resize(partialScansCount);

    for (int i = 0; i < partialScansCount; ++i) {
        QStringList fileNames = QFileDialog::getOpenFileNames(this,
            "Open files",
            QDir::currentPath(),
            "Images (*.png *.xpm *.jpg);;All files (*.*)");

        partialScans[i].loadFromFiles(fileNames);
    }
}

void MainWindow::on_sliceSpinBox_valueChanged(int arg1) {
    switch (ui->slicePlaneBox->currentIndex()) {
        case 0:
            ui->display->setPixmap(partialScans[0].getXSlice(arg1));
            break;

        case 1:
            ui->display->setPixmap(partialScans[0].getYSlice(arg1));
            break;

        case 2:
            ui->display->setPixmap(partialScans[0].getZSlice(arg1));
            break;
    }
}
