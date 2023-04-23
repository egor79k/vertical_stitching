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

    estimateStitchParams(scan_1, scan_2);
    auto params_1 = scan_1.getEstStitchParams();
    auto params_2 = scan_2.getEstStitchParams();
    int overlap = size_1.z - params_2.offsetZ;
    // int offsetY = params_2.offsetY - params_1.offsetY;
    // int offsetX = params_2.offsetX - params_1.offsetX;

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z - overlap};

    float* stitchedData = new float[stitchedSize.volume()];

    int offsetVolume = overlap * size_2.x * size_2.y;
    int scanVolume = size_2.x * size_2.y * (size_2.z - overlap);
    int layerSpace = size_1.x * size_1.y;

    float* data_2 = scan_2.getData();

    auto stitchedRange = getStitchedRange(scan_1, scan_2);

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    // memcpy(stitchedData + size_1.volume(), data_2 + offsetVolume, scanVolume * sizeof(float));

    for (int z = 0; z < size_2.z - overlap; ++z) {
        for (int y = 0; y < size_2.y; ++y) {
            for (int x = 0; x < size_2.x; ++x) {
                int x2 = x + params_2.offsetX;
                int y2 = y + params_2.offsetY;
                if (x2 >= 0 || x2 < size_2.x || y2 >= 0 || y2 < size_2.y) {
                    stitchedData[(z + size_1.z) * layerSpace + y * size_1.x + x] = data_2[(z + overlap) * layerSpace + y2 * size_1.x + x2];
                }
                else {
                    stitchedData[(z + size_1.z) * layerSpace + y * size_1.x + x] = stitchedRange.min;
                }
            }
        }
    }

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, stitchedRange, scan_1.getRefStitchParams());
}


std::shared_ptr<VoxelContainer> StitcherImpl::stitch(std::vector<std::shared_ptr<VoxelContainer>>& partialScans) {
    std::shared_ptr<VoxelContainer> result = partialScans[0];
    result->setEstStitchParams({0, 0, 0});

    for (int scan_id = 1; scan_id < partialScans.size(); ++scan_id) {
        result = stitch(*result, *partialScans[scan_id]);
        
        if (result == nullptr) {
            return nullptr;
        }
    }
    
    return result;
}


VoxelContainer::Range StitcherImpl::getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Range r1 = scan_1.getRange();
    VoxelContainer::Range r2 = scan_2.getRange();

    return {std::min(r1.min, r2.min), std::max(r1.max, r2.max)};
}
