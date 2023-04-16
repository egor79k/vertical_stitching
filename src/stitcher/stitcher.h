#ifndef STITCHER_H
#define STITCHER_H

#include <memory>
#include <vector>
#include "voxel_container.h"


class StitcherImpl {
public:
    virtual std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2);
    std::shared_ptr<VoxelContainer> stitch(std::vector<std::shared_ptr<VoxelContainer>>& partialScans);

protected:
    virtual int determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) = 0;
    VoxelContainer::Range getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2);
};


#endif // STITCHER_H