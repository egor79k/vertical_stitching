#ifndef VOXELCONTAINER_H
#define VOXELCONTAINER_H

#include <QStringList>
#include <QPixmap>


class VoxelContainer {
public:
    VoxelContainer() = default;
    ~VoxelContainer();

    bool loadFromFiles(const QStringList& fileNames);

    QPixmap getXSlice(const int sliceId); // Sagittal plane
    QPixmap getYSlice(const int sliceId); // Coronal plane
    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    qsizetype xSize = 0;
    qsizetype ySize = 0;
    qsizetype zSize = 0;
    int bytesPerPixel = 1;
    QImage::Format format;
    uchar* data = nullptr;
};

#endif // VOXELCONTAINER_H
