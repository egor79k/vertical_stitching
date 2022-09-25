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

        qsizetype volume();
    };
    
    VoxelContainer() = default;
    VoxelContainer(const QStringList& fileNames);
    VoxelContainer(uchar* _data, const Vector3& _size);
    ~VoxelContainer();

    bool loadFromFiles(const QStringList& fileNames);

    bool isEmpty();

    const Vector3& getSize() const;
    const uchar* getData() const;
    QPixmap getXSlice(const int sliceId); // Sagittal plane
    QPixmap getYSlice(const int sliceId); // Coronal plane
    QPixmap getZSlice(const int sliceId); // Transverse plane
//    QPixmap getSlice(const int planeId, const int sliceId);

private:
    Vector3 size = {0, 0, 0};
    int bytesPerPixel = 1;
//    QImage::Format format;
    uchar* data = nullptr;
};

#endif // VOXELCONTAINER_H
