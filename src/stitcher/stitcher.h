#ifndef STITCHER_H
#define STITCHER_H

#include <memory>
#include <vector>
#include "voxel_container.h"


/// Abstract base stitcher class.
class StitcherImpl {
public:
    /**
     * \brief Stitches two reconstructions into one.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     * \return Shared pointer to the new stitched reconstruction.
     */
    virtual std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2);

    /**
     * \brief Stitches several reconstructions into one.
     * 
     * \param[in] partialScans Vector of shared pointers to reconstructions to be stitched
     * \return Shared pointer to the new stitched reconstruction.
     */
    std::shared_ptr<VoxelContainer> stitch(std::vector<std::shared_ptr<VoxelContainer>>& partialScans);

protected:
    /**
     * \brief Gives estimated stitch params.
     * 
     * Must be overrided to determine stitch parameters using particular
     * algorithm in the inheritor. Result must be stored into the estimated
     * stitch parameters of the scan_2. Use
     * VoxelContainer::setEstStitchParams() to set it.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     */
    virtual void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) = 0;

    /**
     * \brief Gives the common range of two different reconstruction.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     * \return Common range.
     */
    VoxelContainer::Range getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2);
};


#endif // STITCHER_H