#include <QApplication>
#include <QList>
#include "ui/mainwindow.h"


int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    AlgoList stitchAlgos = {
        {std::make_shared<OverlapDifferenceStitcher>(), "Intersection difference"},
        {std::make_shared<SimpleStitcher>(), "Without overlay"}};

    MainWindow w(&stitchAlgos);
    w.setWindowTitle("Vertical stitcher");
    w.show();
    return a.exec();
}
