#include <QDebug>
#include <QImage>
#include "voxel_container.h"

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

    xSize = img.width();
    ySize = img.height();
    zSize = fileNames.size();
    bytesPerPixel = img.depth() / 8;
    format = img.format();

    for (int i = 0; i < zSize; ++i) {
        img.load(fileNames.at(i));

        if (img.width() != xSize || img.height() != ySize) {
            return false;
        }

        memcpy(data + i * img.sizeInBytes(), img.bits(), img.sizeInBytes());
    }

    return true;
}


QPixmap VoxelContainer::getXSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(ySize, zSize, format);
    uchar* bits = img.bits();

    for (int z = 0; z < zSize; ++z) {
        for (int y = 0; y < ySize; ++y) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * ySize + y) * bytesPerPixel + byte] = data[(z * xSize * ySize + sliceId * xSize + y) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getYSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(xSize, zSize, format);
    uchar* bits = img.bits();

    for (int z = 0; z < zSize; ++z) {
        for (int x = 0; x < xSize; ++x) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * xSize + x) * bytesPerPixel + byte] = data[(z * xSize * ySize + sliceId * ySize + x) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getZSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    return QPixmap::fromImage(QImage(data + sliceId * xSize * ySize * bytesPerPixel, xSize, ySize, xSize * bytesPerPixel, format));
}
