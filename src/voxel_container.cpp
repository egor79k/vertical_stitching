#include <QDebug>
#include <QImage>
#include "voxel_container.h"


qsizetype VoxelContainer::Vector3::volume() {
    return x * y * z;
}


VoxelContainer::VoxelContainer(const QStringList& fileNames) {
    loadFromFiles(fileNames);
}


VoxelContainer::VoxelContainer(uchar* _data, const Vector3& _size) :
    data(_data),
    size(_size) {}


VoxelContainer::~VoxelContainer() {
    if (data != nullptr) {
        delete[] data;
        size = {0, 0, 0};
    }
}


bool VoxelContainer::loadFromFiles(const QStringList& fileNames) {
    QImage img(fileNames.first());

    if (img.isNull()) {
        return false;
    }

    clear();

    data = new uchar[fileNames.size() * img.sizeInBytes()];

    size.x = img.width();
    size.y = img.height();
    size.z = fileNames.size();
//    bytesPerPixel = img.depth() / 8;
//    format = img.format();

    for (int i = 0; i < size.z; ++i) {
        img.load(fileNames.at(i));

        if (img.format() != QImage::Format_Grayscale8) {
            img = img.convertToFormat(QImage::Format_Grayscale8);
        }

        if (img.width() != size.x || img.height() != size.y) {
            delete[] data;
            data = nullptr;
            return false;
        }

        memcpy(data + i * img.sizeInBytes(), img.bits(), img.sizeInBytes());
    }

    return true;
}


void VoxelContainer::clear() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
        size = {0, 0, 0};
    }
}


bool VoxelContainer::isEmpty() {
    return data == nullptr;
}


const VoxelContainer::Vector3& VoxelContainer::getSize() const {
    return size;
}


const uchar* VoxelContainer::getData() const {
    return data;
}


QPixmap VoxelContainer::getSlice(const int planeId, const int sliceId) {
    if (data == nullptr) {
        return QPixmap();
    }

    switch (planeId) {
        case 0: {
            QImage img(size.y, size.z, QImage::Format_Grayscale8);
            uchar* bits = img.bits();

            for (int z = 0; z < size.z; ++z) {
                for (int y = 0; y < size.y; ++y) {
                    for (int byte = 0; byte < bytesPerPixel; ++byte) {
                        bits[(z * size.y + y) * bytesPerPixel + byte] = data[(z * size.x * size.y + y * size.x + sliceId) * bytesPerPixel + byte];
                    }
                }
            }

            return QPixmap::fromImage(img);
        }

        case 1: {
            QImage img(size.x, size.z, QImage::Format_Grayscale8);
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

        case 2: {
            return QPixmap::fromImage(
                QImage(data + sliceId * size.x * size.y * bytesPerPixel,
                size.x, size.y, size.x * bytesPerPixel,
                QImage::Format_Grayscale8));
        }

        default:
            return QPixmap();
    }
}


/*
QPixmap VoxelContainer::getXSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(size.y, size.z, QImage::Format_Grayscale8);
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
    QImage img(size.x, size.z, QImage::Format_Grayscale8);
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
    return QPixmap::fromImage(QImage(data + sliceId * size.x * size.y * bytesPerPixel, size.x, size.y, size.x * bytesPerPixel, QImage::Format_Grayscale8));
}
*/
