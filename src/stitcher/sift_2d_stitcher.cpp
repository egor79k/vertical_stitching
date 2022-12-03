#include <cmath>
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
    const int octaves_num = 4;
    const double scale_levels_num = 5;
    double sigma = 1.6;
    const double k = std::pow(std::sqrt(2), 1 / scale_levels_num);
    
    std::vector<std::vector<cv::Mat>> gaussians(octaves_num);
    std::vector<std::vector<cv::Mat>> DoG(octaves_num);

    for (int i = 0; i < octaves_num; ++i) {
        gaussians[i].resize(scale_levels_num);
        DoG[i].resize(scale_levels_num - 1);
    }

    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);

    for (int octave = 0; octave < octaves_num; ++octave) {
        for (int scale_level = 0; scale_level < scale_levels_num; ++scale_level) {
            printf("%s %i %s %i\n", "Octave:", octave, "Scale level:", scale_level);
            sigma *= k;
            cv::GaussianBlur(img, gaussians[octave][scale_level], cv::Size(5, 5), sigma);
            // cv::normalize(gaussians[octave][scale_level], gaussians[octave][scale_level], 0, 1, cv::NORM_MINMAX);
            // cv::imshow("Display Keypoints", gaussians[octave][scale_level]);
            // int key = -1;
            // while (key != 'q') key = cv::waitKeyEx(100);
        }

        cv::resize(img, img, cv::Size(), 0.5, 0.5);
    }

    // cv::destroyAllWindows();

    for (int octave = 0; octave < octaves_num; ++ octave) {
        for (int scale_level = 0; scale_level < scale_levels_num - 1; ++scale_level) {
            DoG[octave][scale_level] = gaussians[octave][scale_level] - gaussians[octave][scale_level + 1];
            cv::normalize(DoG[octave][scale_level], DoG[octave][scale_level], 0, 1, cv::NORM_MINMAX);
            cv::imshow("Display Keypoints", DoG[octave][scale_level]);
            int key = -1;
            while (key != 'q') key = cv::waitKeyEx(100);
        }
    }

    cv::destroyAllWindows();

    // for (int octave = 0; octave < octaves_num; ++ octave) {
    //     for (int scale_level = 1; scale_level < scale_levels_num - 2; ++scale_level) {
    //         for (int y = 1; y < DoG[octave][scale_level].rows - 1; ++y) {
    //             for (int x = 1; x < DoG[octave][scale_level].cols - 1; ++x) {
    //                 float candidate = DoG[octave][scale_level].at(y, x);
    //                 if (candidate > DoG[octave][scale_level - 1].at(y, x)) {
    //                     printf("%i %i", y, x);
    //                 }
    //             }
    //         }
    //     }
    // }
}