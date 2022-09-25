#include <QDebug>
#include <QImage>
#include "voxel_container.h"


VoxelContainer::VoxelContainer(const QStringList& fileNames) {
    loadFromFiles(fileNames);
}


VoxelContainer::~VoxelContainer() {
    if (data != nullptr) {
        delete[] data;
    }
}


bool VoxelContainer::loadFromFiles(const QStringList& fileNames) {
    QImage img(fileNames.first());

    if (img.isNull()) {
        return false;
    }

    if (data != nullptr) {
        delete[] data;
        data = nullptr;
    }

    qDebug() << "\nPixels: " << img.width() << " " << img.height() << " " << img.sizeInBytes() << " " << img.depth();

    data = new uchar[fileNames.size() * img.sizeInBytes()];

    size.x = img.width();
    size.y = img.height();
    size.z = fileNames.size();
    bytesPerPixel = img.depth() / 8;
    format = img.format();

    for (int i = 0; i < size.z; ++i) {
        img.load(fileNames.at(i));

        if (img.width() != size.x || img.height() != size.y) {
            return false;
        }

        memcpy(data + i * img.sizeInBytes(), img.bits(), img.sizeInBytes());
    }

    return true;
}


const VoxelContainer::Vector3& VoxelContainer::getSize() {
    return size;
}


bool VoxelContainer::isEmpty() {
    return data == nullptr;
}


QPixmap VoxelContainer::getXSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(size.y, size.z, format);
    uchar* bits = img.bits();

    for (int z = 0; z < size.z; ++z) {
        for (int y = 0; y < size.y; ++y) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * size.y + y) * bytesPerPixel + byte] = data[(z * size.x * size.y + sliceId * size.x + y) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getYSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(size.x, size.z, format);
    uchar* bits = img.bits();

    for (int z = 0; z < size.z; ++z) {
        for (int x = 0; x < size.x; ++x) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * size.x + x) * bytesPerPixel + byte] = data[(z * size.x * size.y + sliceId * size.y + x) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getZSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    return QPixmap::fromImage(QImage(data + sliceId * size.x * size.y * bytesPerPixel, size.x, size.y, size.x * bytesPerPixel, format));
}
