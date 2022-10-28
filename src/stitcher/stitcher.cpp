#include <QMessageBox>
#include <limits>
#include "stitcher.h"

//QSharedPointer<VoxelContainer> StitcherImpl::stitch(const QList<std::shared_ptr<VoxelContainer>>& partialScans) {}


std::shared_ptr<VoxelContainer> SimpleStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
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

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize);
}


const int OverlapDifferenceStitcher::minOverlap = 2;
const int OverlapDifferenceStitcher::maxOverlap = 10;
const int OverlapDifferenceStitcher::offsetStep = 1;


float OverlapDifferenceStitcher::countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();

    const float* data_1 = scan_1.getData();
    const float* data_2 = scan_2.getData();

    float diff = 0;

    int scanOffset = size_1.x * size_1.y * (size_1.z - overlap);
    int offsetVolume = overlap * size_1.x * size_1.y;

    for (int i = 0; i < offsetVolume; ++i) {
        diff += (data_1[i + scanOffset] - data_2[i]) * (data_1[i + scanOffset] - data_2[i]);
    }

    return diff;
}


int OverlapDifferenceStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    float minDiff = std::numeric_limits<float>::max();
    int optimalOverlap = 0;

    for (int overlap = minOverlap; overlap < maxOverlap; overlap += offsetStep) {
        float currDiff = countDifference(scan_1, scan_2, overlap);
        if (currDiff < minDiff) {
            minDiff = currDiff;
            optimalOverlap = overlap;
        }
    }

    return optimalOverlap;
}


std::shared_ptr<VoxelContainer> OverlapDifferenceStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    int overlap = determineOptimalOverlap(scan_1, scan_2);

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z - overlap};

    float* stitchedData = new float[stitchedSize.volume()];

    int offsetVolume = overlap * size_2.x * size_2.y;
    int scanVolume = size_2.x * size_2.y * (size_2.z - overlap);

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    memcpy(stitchedData + size_1.volume(), scan_2.getData() + offsetVolume, scanVolume * sizeof(float));

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize);
}
