#ifndef SEPARATION_STITCHER_H
#define SEPARATION_STITCHER_H

#include "stitcher.h"


class SeparationStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;

protected:
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


#endif // SEPARATION_STITCHER_H