#include "mainwindow.h"

#include <QApplication>


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
//    SimpleStitcher stitcher;
    OverlapDifferenceStitcher stitcher;
    MainWindow w(&stitcher);
    w.setWindowTitle("Vertical stitcher");
    w.show();
    return a.exec();
}
