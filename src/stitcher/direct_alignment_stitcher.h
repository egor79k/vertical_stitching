#ifndef DIRECT_ALIGNMENT_STITCHER_H
#define DIRECT_ALIGNMENT_STITCHER_H

#include "stitcher.h"

/**
 * \brief Abstract base stitcher class for all direct alignment algorithms.
 * 
 * This class implements the base logic of direct (or pixel) alignment
 * stitching algorithms moving one reconstruction relatively to another one and
 * calculating some metric on its overlay volume. In this version only vertical
 * movement is implemented. Metrics can be defined in the inheritors by
 * overloading the kernel(const float, const float) function.
 */
class DirectAlignmentStitcher : public StitcherImpl {
protected:
    /**
     * \brief Implements the particular metrics. Must be overrided.
     * 
     * \param[in] a First voxel value
     * \param[in] b Second voxel value
     * \return Metrics value.
     */
    virtual float kernel(const float a, const float b) = 0;

    /**
     * \brief Calculates kernel() metric on two reconstructions overlap.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     * \param[in] overlap Vertical offset of one reconstruction relatively to another
     * \return Common range.
     */
    float countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap);

    /**
     * \brief Overrides StitcherImpl::estimateStitchParams(). Implements direct alignment algorithm.
     * 
     * Goes through range of vertical overlaps searching for the optimal one
     * which gives the smallest countDifference() result.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     */
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


class L2DirectAlignmentStitcher : public DirectAlignmentStitcher {
protected:
    /**
     * \brief Overrides DirectAlignmentStitcher::kernel(). Calculates L2 metrics.
     * 
     * \param[in] a First voxel value
     * \param[in] b Second voxel value
     * \return Metrics value.
     */
    float kernel(const float a, const float b) override;
};


#endif // DIRECT_ALIGNMENT_STITCHER_H