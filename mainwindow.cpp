#include <QFileDialog>
#include <QDebug>

#include "mainwindow.h"
#include "./ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
}


MainWindow::~MainWindow() {
    delete ui;
}


void MainWindow::on_fileLoadButton_clicked() {
    QString fileName = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("Open file"),
        QDir::currentPath(),
        "Images (*.tiff *.png *.jpg);;All files (*.*)");

    img.load(fileName);

    ui->display->setPixmap(img);
}
