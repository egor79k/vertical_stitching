#ifndef STITCHER_H
#define STITCHER_H

#include <QSharedPointer>
#include <QVector>
#include "voxel_container.h"


class StitcherImpl {
public:
    virtual QSharedPointer<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) = 0;
    QSharedPointer<VoxelContainer> stitch(const QList<QSharedPointer<VoxelContainer>>& partialScans);
};


class SimpleStitcher : public StitcherImpl {
    QSharedPointer<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;
};

#endif // STITCHER_H
