#ifndef STITCHER_H
#define STITCHER_H

#include <memory>
#include <vector>
#include "voxel_container.h"


class StitcherImpl {
public:
    virtual std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) = 0;
    std::shared_ptr<VoxelContainer> stitch(const std::vector<std::shared_ptr<VoxelContainer>>& partialScans);
    VoxelContainer::Range getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2);
};


class SimpleStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;
};


class OverlapDifferenceStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;

private:
    float countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap);
    int determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2);

    static const int minOverlap;
    static const int maxOverlap;
    static const int offsetStep;
};


class SIFT2DStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;

private:
    // void filterMatches(const std::vector<cv::DMatch>& matches, std::vector<cv::DMatch>& goodMatches);
    int determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2);

    static const int minOverlap;
    static const int maxOverlap;
    static const int offsetStep;
};

#endif // STITCHER_H
