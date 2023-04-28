#ifndef OPENCV_SIFT_2D_STITCHER_H
#define OPENCV_SIFT_2D_STITCHER_H

#include "stitcher.h"


class OpenCVSIFT2DStitcher : public StitcherImpl {
protected:
    float getMedian(std::vector<float>& array);
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


#endif // OPENCV_SIFT_2D_STITCHER_H