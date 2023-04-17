#ifndef DIRECT_ALIGNMENT_STITCHER_H
#define DIRECT_ALIGNMENT_STITCHER_H

#include "stitcher.h"


class DirectAlignmentStitcher : public StitcherImpl {
protected:
    virtual float kernel(const float a, const float b) = 0;
    float countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap);
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


class L2DirectAlignmentStitcher : public DirectAlignmentStitcher {
protected:
    float kernel(const float a, const float b) override;
};


#endif // DIRECT_ALIGNMENT_STITCHER_H