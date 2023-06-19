#ifndef SEPARATION_STITCHER_H
#define SEPARATION_STITCHER_H

#include "stitcher.h"

/// brief Stitcher class uniting reconstructions without any transformations for demonstration.
class SeparationStitcher : public StitcherImpl {
public:
    /**
     * \brief Unites two reconstructions into one with a small gap between them.
     * 
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     * \return Shared pointer to the new stitched reconstruction.
     */
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;

protected:

    /**
     * \brief Overrides StitcherImpl::estimateStitchParams().
     * \param[in] scan_1 First reconstruction
     * \param[in] scan_1 Second reconstruction
     */
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) override;
};


#endif // SEPARATION_STITCHER_H