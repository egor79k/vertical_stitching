#include "separation_stitcher.h"

std::shared_ptr<VoxelContainer> SeparationStitcher::stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();
    VoxelContainer::Vector3 gap = {size_1.x, size_1.y, 5};

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        // QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z + gap.z};
    VoxelContainer::Range stitchedRange = getStitchedRange(scan_1, scan_2);
    float* stitchedData = new float[stitchedSize.volume()];

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));

    // Fill the gap with black
    float* gapData = stitchedData + size_1.volume();
    for (int i = 0; i < gap.volume(); ++ i) {
        gapData[i] = stitchedRange.min;
    }

    memcpy(stitchedData + size_1.volume() + gap.volume(), scan_2.getData(), size_2.volume() * sizeof(float));

    scan_2.setEstStitchParams({0, 0, static_cast<int>(size_1.z) + 5});

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, stitchedRange);
}


void SeparationStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {}