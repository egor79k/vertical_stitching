#include <fstream>
#include <iterator>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <nlohmann/json.hpp>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "tiff_image.h"

using json = nlohmann::json;


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


void MainWindow::updateSeamHighlight(int state) {
    // Clear all existing highlight
    for (QGraphicsRectItem* rectItem : seamHighlights) {
        displayScene.removeItem(rectItem);
    }

    seamHighlights.clear();

    int plane = ui->slicePlaneBox->currentIndex();

    // If checked create new highlight
    if (state > 0 && partialScans.size() > 1 && plane != 2) {
        int width = 0;

        if (plane == 1) {
            width = partialScans[0]->getSize().x;
        }
        else {
            width = partialScans[0]->getSize().y;
        }

        for (int i = 1; i < partialScans.size(); ++i) {
            auto prevSize = partialScans[i - 1]->getSize();
            int prevOffset = partialScans[i - 1]->getEstStitchParams().offsetZ;
            int currOffset = partialScans[i]->getEstStitchParams().offsetZ;
            int seamStart = prevOffset + prevSize.z;
            
            if (seamStart < currOffset) {
                qDebug() << "Overlap:" << seamStart << currOffset - seamStart;
                QBrush brush = QBrush({255, 255, 255, 255});
                QGraphicsRectItem* rectItem = displayScene.addRect(0, seamStart, width, currOffset - seamStart, QPen({0, 0, 0, 0}), brush);
                seamHighlights.push_back(rectItem);
            }
            else {
                QBrush brush = QBrush({255, 0, 0, 40});
                QGraphicsRectItem* rectItem = displayScene.addRect(0, currOffset, width, seamStart - currOffset, QPen({0, 0, 0, 0}), brush);
                seamHighlights.push_back(rectItem);
            }
        }
    }
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
    if (partialScans.size() > 0) {
        stitchedScan = stitcher->stitch(partialScans);

        if (stitchedScan == nullptr) {
            return;
        }
    }
    else {
        stitchedScan->clear();
    }

    int plane = ui->slicePlaneBox->currentIndex();
    updateSliceBounds(plane);
    updateDisplay(plane, ui->sliceSpinBox->value());
    updateSeamHighlight(ui->seamHighlightCheckBox->isChecked());
}


void MainWindow::appendScansList() {
    auto scanShape = partialScans.back()->getSize();

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

    // Convert QStringList to std::vector<std::string>
    std::vector<std::string> fileNamesStd(fileNames.size());

    for (int i = 0; i < fileNames.size(); ++i) {
        fileNamesStd[i] = fileNames[i].toStdString();
    }

    if (fileNames.size() == 1 && fileNames.back().endsWith(".json")) {
        // Open parameters file
        std::string& json_file = fileNamesStd.back();

        std::ifstream fs(json_file);

        if(!fs) {
            QMessageBox::information(nullptr, "File error", QString("Unable to read JSON file '%1'").arg(json_file.data()));
            return;
        }

        // Parse JSON parameters
        json data = json::parse(fs, nullptr, false);

        if (data.is_discarded()) {
            QMessageBox::information(nullptr, "Parse error", QString("Unable to parse JSON file '%1'").arg(json_file.data()));
            return;
        }

        if (data.contains("parts_num")) {
            // Try to load several reconstructions from common parameters file
            int parts_num = data["parts_num"].get<int>();
            std::string param_path = json_file.substr(0, json_file.find_last_of('/') + 1);

            for (int part_id = 0; part_id < parts_num; ++part_id) {
                // Try to load part reconstruction from parameters
                partialScans.emplace_back(std::make_shared<VoxelContainer>());
                std::string part_info_file = param_path + std::to_string(part_id) + "/info.json";

                if (!partialScans.back()->loadFromJson(part_info_file)) {
                    partialScans.pop_back();
                    QMessageBox::information(nullptr, "Load error", QString("Unable to load from JSON file '%1'").arg(part_info_file.data()));
                    return;
                }

                appendScansList();
            }
        }
        else {
            // Try to load reconstruction from parameters
            partialScans.emplace_back(std::make_shared<VoxelContainer>());

            if (!partialScans.back()->loadFromJson(json_file)) {
                partialScans.pop_back();
                QMessageBox::information(nullptr, "Load error", QString("Unable to load from JSON file '%1'").arg(json_file.data()));
                return;
            }

            appendScansList();
        }
    }
    else {
        // Try to load reconstruction from chosen images
        partialScans.emplace_back(std::make_shared<VoxelContainer>());

        if (!partialScans.back()->loadFromImages(fileNamesStd)) {
            partialScans.pop_back();
            QMessageBox::information(nullptr, "Load error", QString("Unable to load from image files"));
            return;
        }

        appendScansList();
    }

    updateStitch();
}


void MainWindow::on_sliceSpinBox_valueChanged(int slice) {
    updateDisplay(ui->slicePlaneBox->currentIndex(), slice);
}


void MainWindow::on_slicePlaneBox_currentIndexChanged(int plane) {
    updateSliceBounds(plane);
    updateDisplay(plane, ui->sliceSpinBox->value());
    updateSeamHighlight(ui->seamHighlightCheckBox->isChecked());
}


void MainWindow::on_scansListrowsMoved(const QModelIndex& parent, int start, int end, const QModelIndex& destination, int row) {
    if (start < row) {
        --row;
    }

    auto temp = partialScans[start];
    partialScans.erase(std::next(partialScans.begin(), start));
    partialScans.insert(std::next(partialScans.begin(), row), temp);
    // partialScans.move(start, row);

    updateStitch();
}


void MainWindow::on_removeScanButton_clicked()
{
    int currentScanId = ui->scansList->currentRow();

    if (currentScanId >= 0) {
        ui->scansList->takeItem(currentScanId);
        partialScans.erase(std::next(partialScans.begin(), currentScanId));
        updateStitch();
    }

}


void MainWindow::on_algorithmBox_currentIndexChanged(int index) {
    stitcher = stitchAlgos->at(index).first;
    updateStitch();
}


void MainWindow::on_actionSave_triggered() {
    if (stitchedScan->isEmpty()) {
        return;
    }

    QString dirName = QFileDialog::getSaveFileName(this,
        "Save file",
        QDir::currentPath(),
        "All files (*.*)");

    if (dirName.isEmpty()) {
        return;
    }

    if (!stitchedScan->saveToJson(dirName.toStdString())) {
        QMessageBox::information(nullptr, "Save error", QString("An error occured while image writing."));
    }

}


void MainWindow::on_actionSaveSlice_triggered() {
    if (stitchedScan->isEmpty()) {
        return;
    }

    QFileDialog dialog(this,
        "Save file",
        QDir::currentPath(),
        "All files (*.*);;Images (*.png *.tiff *.jpg)");
    
    dialog.setDefaultSuffix(".tiff");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    
    if (!dialog.exec()) {
        return;
    }
    
    const auto fileName = dialog.selectedFiles().front();

    if (fileName.isEmpty()) {
        return;
    }

    int plane = ui->slicePlaneBox->currentIndex();
    int slice = ui->sliceSpinBox->value();
    TiffImage<float> img;
    stitchedScan->getSlice<float>(img, plane, slice, true);
    img.save(fileName.toStdString().c_str());
}


void MainWindow::on_actionExportSlice_triggered() {
    if (stitchedScan->isEmpty()) {
        return;
    }

    QFileDialog dialog(this,
        "Save file",
        QDir::currentPath(),
        "All files (*.*);;Images (*.png *.tiff *.jpg)");
    
    dialog.setDefaultSuffix(".tiff");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    
    if (!dialog.exec()) {
        return;
    }
    
    const auto fileName = dialog.selectedFiles().front();

    if (fileName.isEmpty()) {
        return;
    }

    int plane = ui->slicePlaneBox->currentIndex();
    int slice = ui->sliceSpinBox->value();
    TiffImage<uint8_t> img;
    stitchedScan->getSlice<uint8_t>(img, plane, slice, true);
    img.saveAs(fileName.toStdString().c_str());
}


void MainWindow::on_seamHighlightCheckBox_stateChanged(int state) {
    updateSeamHighlight(state);
}

