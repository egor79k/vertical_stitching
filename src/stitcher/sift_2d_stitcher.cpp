#include <cmath>
#include "sift_2d_stitcher.h"


std::shared_ptr<VoxelContainer> SIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    TiffImage<unsigned char> sliceImg_1, sliceImg_2;
    std::vector<std::vector<cv::Mat>> DoG_1, DoG_2;
    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;

    for (int plane : planes) {
        int slice_id = size_1.x / 2;

        scan_1.getSlice<unsigned char>(sliceImg_1, plane, slice_id, true);
        scan_2.getSlice<unsigned char>(sliceImg_2, plane, slice_id, true);

        cv::Mat_<unsigned char> slice_1(size_1.z, size_1.y, sliceImg_1.getData());
        cv::Mat_<unsigned char> slice_2(size_2.z, size_2.y, sliceImg_2.getData());

        DoG_1.clear();
        DoG_2.clear();
        keypoints_1.clear();
        keypoints_2.clear();

        buildDoG(slice_1, DoG_1);
        buildDoG(slice_2, DoG_2);

        detect(slice_1, DoG_1, keypoints_1);
        detect(slice_2, DoG_2, keypoints_2);

        printf("Finded %lu and %lu candidates to keypoints\n", keypoints_1.size(), keypoints_2.size());

        TiffImage<unsigned char> charSliceImg;
        scan_1.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_1);
        scan_2.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_2);
    }

    return nullptr;
}


void SIFT2DStitcher::displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints) {
    cv::Mat_<unsigned char> slice(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
    cv::Mat rgbSlice;
    cv::cvtColor(slice, rgbSlice, cv::COLOR_GRAY2RGB);
    cv::drawKeypoints(rgbSlice, keypoints, rgbSlice);
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Keypoints", rgbSlice);
    cv::waitKey(0);
    cv::destroyAllWindows();
}


void SIFT2DStitcher::buildDoG(cv::Mat img, std::vector<std::vector<cv::Mat>>& DoG) {
    double sigma = 1.6;
    const double k = std::pow(std::sqrt(2), 1 / scale_levels_num);
    
    std::vector<std::vector<cv::Mat>> gaussians(octaves_num);
    DoG.resize(octaves_num);

    for (int i = 0; i < octaves_num; ++i) {
        gaussians[i].resize(scale_levels_num);
        DoG[i].resize(scale_levels_num - 1);
    }

    // cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);

    for (int octave = 0; octave < octaves_num; ++octave) {
        for (int scale_level = 0; scale_level < scale_levels_num; ++scale_level) {
            cv::GaussianBlur(img, gaussians[octave][scale_level], cv::Size(5, 5), sigma);
            sigma *= k;
            // printf("%s %i %s %i\n", "Octave:", octave, "Scale level:", scale_level);
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
            // cv::normalize(DoG[octave][scale_level], DoG[octave][scale_level], 0, 1, cv::NORM_MINMAX);
            // cv::imshow("Display Keypoints", DoG[octave][scale_level]);
            // int key = -1;
            // while (key != 'q') key = cv::waitKeyEx(100);
        }
    }

    // cv::destroyAllWindows();
}


void SIFT2DStitcher::detect(cv::Mat img, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    struct RegExtr {
        cv::Mat reg{};
        double min_val = 0;
        double max_val = 0;
        cv::Point min_loc{};
        cv::Point max_loc{};
    };

    RegExtr extrs[3] = {};

    for (int octave = 0; octave < octaves_num; ++ octave) {
        for (int scale_level = 1; scale_level < scale_levels_num - 2; ++scale_level) {
            for (int y = 1; y < DoG[octave][scale_level].rows - 1; ++y) {
                for (int x = 1; x < DoG[octave][scale_level].cols - 1; ++x) {
                    cv::Rect2i region(x - 1, y - 1, 3, 3);
                    
                    for (int i = 0; i < 3; ++i) {
                        extrs[i].reg = DoG[octave][scale_level - i + 1](region);
                        cv::minMaxLoc(extrs[i].reg,
                            &(extrs[i].min_val),
                            &(extrs[i].max_val),
                            &(extrs[i].min_loc),
                            &(extrs[i].max_loc));
                    }

                    if (cv::Point(1, 1) == extrs[1].min_loc &&
                        extrs[0].min_val > extrs[1].min_val &&
                        extrs[2].min_val > extrs[1].min_val) {
                        int scale = std::pow(2, octave);
                        keypoints.emplace_back(cv::Point2f(x * scale, y * scale), 0.5f);
                        // printf("MIN LOC: %i %i\n", y, x);
                    }

                    if (cv::Point(1, 1) == extrs[1].max_loc &&
                        extrs[0].max_val < extrs[1].max_val &&
                        extrs[2].max_val < extrs[1].max_val) {
                        int scale = std::pow(2, octave);
                        keypoints.emplace_back(cv::Point2f(x * scale, y * scale), 0.5f);
                        // printf("MAX LOC: %i %i\n", y, x);
                    }
                }
            }
        }
    }
}