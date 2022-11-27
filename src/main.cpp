#include <memory>
#include <QApplication>
#include <QList>
#include "ui/mainwindow.h"
#include "stitcher.h"
#include "sift_2d_stitcher.h"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    AlgoList stitchAlgos = {
        {std::make_shared<SIFT2DStitcher>(), "SIFT 2D"},
        {std::make_shared<CVSIFT2DStitcher>(), "CV SIFT 2D"},
        {std::make_shared<OverlapDifferenceStitcher>(), "Intersection difference"},
        {std::make_shared<SimpleStitcher>(), "Without overlay"}};

    MainWindow w(&stitchAlgos);
    w.setWindowTitle("Vertical stitcher");
    w.show();
    return a.exec();
}
