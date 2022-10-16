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
public:
    QSharedPointer<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;
};


class OverlapDifferenceStitcher : public StitcherImpl {
public:
    QSharedPointer<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;

private:
    float countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap);
    int determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2);

    static const int minOverlap;
    static const int maxOverlap;
    static const int offsetStep;
};

#endif // STITCHER_H
