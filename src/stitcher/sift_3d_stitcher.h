#ifndef SIFT_3D_STITCHER
#define SIFT_3D_STITCHER

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "voxel_container.h"
#include "stitcher.h"


class SIFT3DStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;
    // void testDetection();

private:
    // struct KeyPoint {
    //     int x;
    //     int y;
    //     int z;
    //     int s;
    //     int octave;
    // };

    int determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2);
    void displaySlice(const VoxelContainer& src);
    void gaussianBlur(const VoxelContainer& src, VoxelContainer& dst, const double sigma);
    void compressTwice(const VoxelContainer& src, VoxelContainer& dst);
    void buildDoG(const VoxelContainer& vol, std::vector<std::vector<VoxelContainer>>& gaussians, std::vector<std::vector<VoxelContainer>>& DoG);
    // void detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    // void gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    // void hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    // void localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    // float parabolicInterpolation(float y1, float y2, float y3);
    // void orient(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    // void calculateDescriptors(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat descriptors);

    const int octavesNum = 2;
    const int scaleLevelsNum = 1;
    const int blurLevelsNum = scaleLevelsNum + 3;
    const double sigma = 1.6;
    // std::vector<int> planes = {0, 1, 3, 4};

    // std::vector<std::vector<cv::Mat>> gaussians;
    // std::vector<std::vector<cv::Mat>> DoG;
};

#endif // SIFT_3D_STITCHER