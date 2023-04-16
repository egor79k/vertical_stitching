#include <memory>
#include <QApplication>
#include <QList>
#include "ui/mainwindow.h"
#include "stitcher.h"
#include "separation_stitcher.h"
#include "direct_alignment_stitcher.h"
#include "opencv_sift_2d_stitcher.h"
#include "sift_2d_stitcher.h"
#include "sift_3d_stitcher.h"


int main(int argc, char *argv[]) {
    // SIFT2DStitcher stitcher;
    // stitcher.testDetection();
    // return 0;
    QApplication a(argc, argv);

    AlgoList stitchAlgos = {
        {std::make_shared<SIFT3DStitcher>(), "SIFT 3D"},
        {std::make_shared<SIFT2DStitcher>(), "SIFT 2D"},
        {std::make_shared<OpenCVSIFT2DStitcher>(), "CV SIFT 2D"},
        {std::make_shared<DirectAlignmentStitcher>(), "Intersection difference"},
        {std::make_shared<SeparationStitcher>(), "Without overlay"}};

    MainWindow w(&stitchAlgos);
    w.setWindowTitle("Vertical stitcher");
    w.show();
    return a.exec();
}
