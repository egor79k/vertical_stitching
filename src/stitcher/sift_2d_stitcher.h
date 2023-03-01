#ifndef SIFT_2D_STITCHER
#define SIFT_2D_STITCHER

#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include "voxel_container.h"
#include "stitcher.h"


class SIFT2DStitcher : public StitcherImpl {
public:
    std::shared_ptr<VoxelContainer> stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) override;
    void testDetection();

private:
    struct KeyPoint {
        int x;
        int y;
        int s;
        int octave;
    };

    void displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints);
    void buildDoG(cv::Mat img, std::vector<std::vector<cv::Mat>>& DoG);
    void detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result);
    void localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);
    void orient(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints);

    const int octaves_num = 4;
    const int scale_levels_num = 5;
    const int blur_levels_num = scale_levels_num + 3;
    const double sigma = 1.6;
    std::vector<int> planes = {0, 1, 3, 4};

    std::vector<std::vector<cv::Mat>> gaussians;
    // std::vector<std::vector<cv::Mat>> DoG;
    cv::Mat HoG;
};

#endif // SIFT_2D_STITCHER