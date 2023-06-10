#ifndef SIFT_3D_STITCHER
#define SIFT_3D_STITCHER

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "voxel_container.h"
#include "stitcher.h"


class SIFT3DStitcher : public StitcherImpl {
private:
    // struct KeyPoint {
    //     int x;
    //     int y;
    //     int z;
    //     int s;
    //     int octave;
    // };

    float getMedian(std::vector<float>& array);
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2);
    void displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints, const int start, const int end);
    void displayMatches(TiffImage<unsigned char>& sliceImg_1, TiffImage<unsigned char>& sliceImg_2, const std::vector<cv::KeyPoint>& keypoints_1, const std::vector<cv::KeyPoint>& keypoints_2, const std::vector<cv::DMatch>& matches, const int maxOverlap);
    void displaySlice(const VoxelContainer& src);
    void gaussianBlur(const VoxelContainer& src, VoxelContainer& dst, const double sigma, const int start = 0, const int end = 0);
    void compressTwice(const VoxelContainer& src, VoxelContainer& dst);
    void buildDoG(const VoxelContainer& vol, std::vector<std::vector<VoxelContainer>>& gaussians, std::vector<std::vector<VoxelContainer>>& DoG, const int start, const int end);
    void detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    float parabolicInterpolation(float y1, float y2, float y3);
    void orient(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void calculateDescriptors(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

    int octavesNum = 3;
    const int scaleLevelsNum = 3;
    const int blurLevelsNum = scaleLevelsNum + 3;
    double sigma = 0.9;
    std::vector<std::pair<int, float>> planes = {{0, 0.4}, {0, 0.5}, {0, 0.6}, {1, 0.4}, {1, 0.5}, {1, 0.6}, {3, 0}, {4, 0}};
};

#endif // SIFT_3D_STITCHER