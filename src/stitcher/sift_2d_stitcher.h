#ifndef SIFT_2D_STITCHER
#define SIFT_2D_STITCHER

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "voxel_container.h"
#include "stitcher.h"


class SIFT2DStitcher : public StitcherImpl {
public:
    // TEMP FUNCTION FOR TESTING ON 2D IMAGES
    void testDetection(const char* img_path_1, const char* img_path_2);

private:
    struct KeyPoint {
        int x;
        int y;
        int s;
        int octave;
    };

    float getMedian(std::vector<float>& array);
    void estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2);
    void displayKeypoints(const cv::Mat& sliceImg, const std::vector<cv::KeyPoint>& keypoints);
    void displayMatches(const cv::Mat& slice_1, const cv::Mat& slice_2, const std::vector<cv::KeyPoint>& keypoints_1, const std::vector<cv::KeyPoint>& keypoints_2, const std::vector<cv::DMatch>& matches);
    void buildDoG(cv::Mat img, std::vector<std::vector<cv::Mat>>& gaussians, std::vector<std::vector<cv::Mat>>& DoG);
    void detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    float parabolicInterpolation(float y1, float y2, float y3);
    void orient(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void calculateDescriptors(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors);

    int octaves_num = 4;
    const int scale_levels_num = 3;
    const int blur_levels_num = scale_levels_num + 3;
    double sigma = 1.6;
    // std::vector<int> planes = {0, 1, 3, 4};
    std::vector<std::pair<int, float>> planes = {{0, 0.4}, {0, 0.5}, {0, 0.6}, {1, 0.4}, {1, 0.5}, {1, 0.6}, {3, 0}, {4, 0}};
};

#endif // SIFT_2D_STITCHER