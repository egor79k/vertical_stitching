#include "sift_2d_stitcher.h"


std::shared_ptr<VoxelContainer> SIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    TiffImage<float> img_1;
    scan_1.getSlice<float>(img_1, 0, size_1.x / 2, false);
    cv::Mat_<float> slice_1(size_1.z, size_1.y, img_1.getData());
    detect(slice_1);
    return nullptr;
}


void SIFT2DStitcher::detect(cv::Mat img) {
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    cv::normalize(img, img, 0, 1, cv::NORM_MINMAX);
    cv::imshow("Display Keypoints", img);
    cv::waitKey(0);
    cv::GaussianBlur(img, img, cv::Size(5, 5), 0, 0);
    cv::imshow("Display Keypoints", img);
    cv::waitKey(0);
    cv::destroyAllWindows();

    // std::vector<cv::Mat> gaussians;
}