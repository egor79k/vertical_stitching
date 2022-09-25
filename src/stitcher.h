#ifndef STITCHER_H
#define STITCHER_H

#include <QVector>
#include "voxel_container.h"


class StitcherImpl {
public:
    virtual void stitch(QVector<VoxelContainer>& partialScans, VoxelContainer& stitchedScan) = 0;
};


class SimpleStitcher : public StitcherImpl {
    void stitch(QVector<VoxelContainer>& partialScans, VoxelContainer& stitchedScan) override;
};

#endif // STITCHER_H
