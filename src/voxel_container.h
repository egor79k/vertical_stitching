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
    VoxelContainer(float* _data, const Vector3& _size);
    ~VoxelContainer();

    bool loadFromImages(const QStringList& fileNames);
    bool loadFromJson(const QString &fileName);
    void clear();

    bool isEmpty();

    const Vector3& getSize() const;
    const float* getData() const;
    QPixmap getSlice(const int planeId, const int sliceId);

//    QPixmap getXSlice(const int sliceId); // Sagittal plane
//    QPixmap getYSlice(const int sliceId); // Coronal plane
//    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    void fitToFloatRange(int64_t min, int64_t max);

    template<typename Tin>
    bool readImagesToFloat(const QStringList &fileNames);

    Vector3 size = {0, 0, 0};
    float* data = nullptr;
};

#endif // VOXELCONTAINER_H
