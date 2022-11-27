#ifndef SIFT_2D_STITCHER
#define SIFT_2D_STITCHER

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "voxel_container.h"
#include "stitcher.h"


class SIFT2DStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;

private:
    void detect(cv::Mat img);
};

#endif // SIFT_2D_STITCHER