#include <iostream>
#include <cstring>
#include <limits>
#include <opencv2/opencv.hpp>
#include "stitcher.h"


std::shared_ptr<VoxelContainer> StitcherImpl::stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        // QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    int overlap = determineOptimalOverlap(scan_1, scan_2);

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z - overlap};

    float* stitchedData = new float[stitchedSize.volume()];

    int offsetVolume = overlap * size_2.x * size_2.y;
    int scanVolume = size_2.x * size_2.y * (size_2.z - overlap);

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    memcpy(stitchedData + size_1.volume(), scan_2.getData() + offsetVolume, scanVolume * sizeof(float));

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, getStitchedRange(scan_1, scan_2));
}


std::shared_ptr<VoxelContainer> StitcherImpl::stitch(std::vector<std::shared_ptr<VoxelContainer>>& partialScans) {
    std::shared_ptr<VoxelContainer> result = partialScans[0];
    result->setRefStitchParams(VoxelContainer::StitchParams());
    VoxelContainer::StitchParams prev_params = VoxelContainer::StitchParams();

    for (int scan_id = 1; scan_id < partialScans.size(); ++scan_id) {
        result = stitch(*result, *partialScans[scan_id]);
        
        if (result == nullptr) {
            return nullptr;
        }

        auto params = partialScans[scan_id]->getRefStitchParams();

        prev_params.offsetZ += params.offsetZ;
        prev_params.offsetX += params.offsetX;
        prev_params.offsetY += params.offsetY;

        partialScans[scan_id]->setRefStitchParams(prev_params);
    }

    return result;
}


VoxelContainer::Range StitcherImpl::getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Range r1 = scan_1.getRange();
    VoxelContainer::Range r2 = scan_2.getRange();

    return {std::min(r1.min, r2.min), std::max(r1.max, r2.max)};
}
