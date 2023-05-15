#include "direct_alignment_stitcher.h"


float DirectAlignmentStitcher::countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();

    const float* data_1 = scan_1.getData();
    const float* data_2 = scan_2.getData();

    float diff = 0;

    int scanOffset = size_1.x * size_1.y * (size_1.z - overlap);
    int offsetVolume = overlap * size_1.x * size_1.y;

    for (int i = 0; i < offsetVolume; ++i) {
        diff += kernel(data_1[i + scanOffset], data_2[i]);
        // diff += (data_1[i + scanOffset] - data_2[i]) * (data_1[i + scanOffset] - data_2[i]);
    }

    return diff / overlap;
}


void DirectAlignmentStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    const int offsetStep = 1;
    const int height_1 = scan_1.getSize().z;
    const int height_2 = scan_2.getSize().z;
    const int refOffsetZ = scan_2.getRefStitchParams().offsetZ;
    int maxDeviation = std::min(height_1, height_2) / 4;
    int refOverlap = maxDeviation;

    if (refOffsetZ > 0) {
        refOverlap = height_1 - refOffsetZ;
        maxDeviation = std::max(5, refOverlap / 5);
    }

    float minDiff = std::numeric_limits<float>::max();
    int optimalOverlap = 0;

    for (int overlap = refOverlap - maxDeviation; overlap < refOverlap + maxDeviation; overlap += offsetStep) {
        float currDiff = countDifference(scan_1, scan_2, overlap);
        
        if (currDiff < minDiff) {
            minDiff = currDiff;
            optimalOverlap = overlap;
        }

        printf("%s %i %s %f\n", "Overlap:", overlap, "diff:", currDiff);
    }

    printf("%s %i\n", "Optimal overlap:", optimalOverlap);

    scan_2.setEstStitchParams({0, 0, height_1 - optimalOverlap});

    auto refParams_1 = scan_1.getRefStitchParams();
    auto refParams_2 = scan_2.getRefStitchParams();
    printf("Offsets are 0 0 %i. Should be %i %i %i\n", height_1 - optimalOverlap, refParams_2.offsetX - refParams_1.offsetX, refParams_2.offsetY - refParams_1.offsetY, refParams_2.offsetZ - refParams_1.offsetZ);
}


float L2DirectAlignmentStitcher::kernel(const float a, const float b) {
    return (a - b) * (a - b);
}