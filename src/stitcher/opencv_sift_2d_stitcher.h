#ifndef OPENCV_SIFT_2D_STITCHER_H
#define OPENCV_SIFT_2D_STITCHER_H

#include "stitcher.h"

/**
 * \brief Stitcher class based on the SIFT algorithm from OpenCV.
 * 
 * Logic described in details in estimateStitchParams() function.
 */
class OpenCVSIFT2DStitcher : public StitcherImpl {
protected:
    /**
     * \brief Gives the median of an array.
     * 
     * \param[in] array Input sequence of values
     * \return Median value.
     */
    float getMedian(std::vector<float>& array);

    /**
     * \brief Overrides StitcherImpl::estimateStitchParams(). Implements
     * stitching algorithm based on SIFT from <a href="https://opencv.org/">OpenCV</a> library.
     * 
     * Finds keypoints on the corresponding vertical slice planes and matches
     * them using brute force method. Median of the vertical distance between
     * the matched pairs is taken as an optimal vertical overlap. After that,
     * the corresponding horizontal planes keypoints are matched and similarly
     * horizontal offsets sets as medians of distances between keypoints. This
     * version uses the following vertical planes, (listed as plane id (from VoxelContainer::getSlice()) and slice level):
     * (0, 0.4), (0, 0.5), (0, 0.6), (1, 0.4), (1, 0.5), (1, 0.6), (3, 0), (4, 0).
     * And horizontal:
     * (2, 0.3), (2, 0.4), (2, 0.5), (2, 0.6), (2, 0.7).
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     */
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


#endif // OPENCV_SIFT_2D_STITCHER_H