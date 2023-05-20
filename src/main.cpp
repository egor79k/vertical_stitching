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
    // for (int octaves_num = 1; octaves_num < 6; ++octaves_num) {
    //     printf("%i 5 1.6\n", octaves_num);
    //     fflush(stdout);
    //     SIFT2DStitcher stitcher(octaves_num, 5, 1.6);
    //     stitcher.testDetection();
    // }

    // for (int scale_levels_num = 1; scale_levels_num < 7; ++scale_levels_num) {
    //     printf("4 %i 1.6\n", scale_levels_num);
    //     fflush(stdout);
    //     SIFT2DStitcher stitcher(4, scale_levels_num, 1.6);
    //     stitcher.testDetection();
    // }

    // for (float sigma = 0.5; sigma < 2.0; sigma += 0.1) {
    //     printf("4 5 %f\n", sigma);
    //     fflush(stdout);
    //     SIFT2DStitcher stitcher(4, 5, sigma);
    //     stitcher.testDetection();
    // }

    // SIFT2DStitcher stitcher;
    // stitcher.testDetection(argv[1], argv[2]);
    // return 0;

    QApplication a(argc, argv);

    AlgoList stitchAlgos = {
        {std::make_shared<SeparationStitcher>(), "Separation"},
        {std::make_shared<L2DirectAlignmentStitcher>(), "L2 direct alignment"},
        {std::make_shared<OpenCVSIFT2DStitcher>(), "OpenCV SIFT 2D"},
        {std::make_shared<SIFT2DStitcher>(), "SIFT 2D"},
        {std::make_shared<SIFT3DStitcher>(), "SIFT 3D"}};

    MainWindow w(&stitchAlgos);
    w.setWindowTitle("Vertical stitcher");
    w.show();
    return a.exec();
}
