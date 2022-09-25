#ifndef VOXELCONTAINER_H
#define VOXELCONTAINER_H

#include <QStringList>
#include <QPixmap>


class VoxelContainer {
public:
    struct Vector3 {
        qsizetype x;
        qsizetype y;
        qsizetype z;
    };
    
    VoxelContainer() = default;
    VoxelContainer(const QStringList& fileNames);
    ~VoxelContainer();

    bool loadFromFiles(const QStringList& fileNames);
    const Vector3& getSize();
    bool isEmpty();

    QPixmap getXSlice(const int sliceId); // Sagittal plane
    QPixmap getYSlice(const int sliceId); // Coronal plane
    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    Vector3 size = {0, 0, 0};
    int bytesPerPixel = 1;
    QImage::Format format;
    uchar* data = nullptr;
};

#endif // VOXELCONTAINER_H
