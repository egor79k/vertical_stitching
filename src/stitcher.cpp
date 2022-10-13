#include <QErrorMessage>
#include <QMessageBox>
#include "stitcher.h"

//QSharedPointer<VoxelContainer> StitcherImpl::stitch(const QList<QSharedPointer<VoxelContainer>>& partialScans) {}


QSharedPointer<VoxelContainer> SimpleStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z};

    float* stitchedData = new float[stitchedSize.volume()];

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    memcpy(stitchedData + size_1.volume(), scan_2.getData(), size_2.volume() * sizeof(float));

    return QSharedPointer<VoxelContainer>::create(stitchedData, stitchedSize);
}
