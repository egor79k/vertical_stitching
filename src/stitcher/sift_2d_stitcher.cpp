#include <cmath>
#include "sift_2d_stitcher.h"


std::shared_ptr<VoxelContainer> SIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    TiffImage<float> sliceImg_1, sliceImg_2;
    std::vector<std::vector<cv::Mat>> DoG_1, DoG_2;
    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;

    for (int plane : planes) {
        int slice_id = size_1.x / 2;

        scan_1.getSlice<float>(sliceImg_1, plane, slice_id, false);
        scan_2.getSlice<float>(sliceImg_2, plane, slice_id, false);

        cv::Mat_<float> slice_1(size_1.z, size_1.y, sliceImg_1.getData());
        cv::Mat_<float> slice_2(size_2.z, size_2.y, sliceImg_2.getData());

        DoG_1.clear();
        DoG_2.clear();
        keypoints_1.clear();
        keypoints_2.clear();

        buildDoG(slice_1, DoG_1);
        buildDoG(slice_2, DoG_2);

        detect(DoG_1, keypoints_1);
        detect(DoG_2, keypoints_2);

        printf("Finded %lu and %lu candidates to keypoints\n", keypoints_1.size(), keypoints_2.size());

        TiffImage<unsigned char> charSliceImg;
        scan_1.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_1);
        scan_2.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_2);
    }

    return nullptr;
}


void SIFT2DStitcher::testDetection() {
    cv::Mat origImg = cv::imread("/home/egor/projects/vertical_stitching/reconstructions/SIFT_test.png");
    cv::Mat_<float> img;
    cv::cvtColor(origImg, origImg, cv::COLOR_RGB2GRAY);
    origImg.convertTo(img, CV_32F);
    cv::normalize(img, img, 0, 1, cv::NORM_MINMAX);
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    // cv::imshow("Display Keypoints", img);
    // cv::waitKey(0);

    std::vector<std::vector<cv::Mat>> DoG_1;
    std::vector<cv::KeyPoint> keypoints_1;
    buildDoG(img, DoG_1);
    detect(DoG_1, keypoints_1);

    printf("Finded %lu candidates to keypoints\n", keypoints_1.size());
    cv::drawKeypoints(origImg, keypoints_1, origImg);
    cv::imshow("Display Keypoints", origImg);
    cv::waitKey(0);
    cv::destroyAllWindows();
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


// TEMP DEBUG FUNCTION
void displayImg(cv::Mat img) {
    cv::Mat normImg;
    cv::normalize(img, normImg, 0, 1, cv::NORM_MINMAX);
    cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Image", normImg);
    cv::waitKey(0);
    cv::destroyAllWindows();
}


void SIFT2DStitcher::buildDoG(cv::Mat img, std::vector<std::vector<cv::Mat>>& DoG) {
    const double sigma = 1.6;
    const double k = std::pow(2, 1 / static_cast<double>(scale_levels_num));
    
    std::vector<std::vector<cv::Mat>> gaussians(octaves_num);
    DoG.resize(octaves_num);

    for (int i = 0; i < octaves_num; ++i) {
        gaussians[i].resize(blur_levels_num);
        DoG[i].resize(blur_levels_num - 1);
    }

    cv::GaussianBlur(img, gaussians[0][0], cv::Size(0, 0), sigma);

    for (int octave = 0; octave < octaves_num; ++octave) {
        // displayImg(gaussians[octave][0]);

        for (int scale_level = 1; scale_level < blur_levels_num; ++scale_level) {
            double kernel = sigma * std::pow(2, octave) * std::pow(k, scale_level);
            cv::GaussianBlur(gaussians[octave][scale_level - 1], gaussians[octave][scale_level], cv::Size(0, 0), kernel);
            // displayImg(gaussians[octave][scale_level]);
        }

        if (octave < octaves_num - 1) {
            cv::resize(gaussians[octave][scale_levels_num - 1], gaussians[octave + 1][0], cv::Size(), 0.5, 0.5, cv::INTER_NEAREST);
        }
    }

    for (int octave = 0; octave < octaves_num; ++ octave) {
        for (int scale_level = 0; scale_level < blur_levels_num - 1; ++scale_level) {
            DoG[octave][scale_level] = gaussians[octave][scale_level] - gaussians[octave][scale_level + 1];
            // displayImg(DoG[octave][scale_level]);
        }
    }
}


void SIFT2DStitcher::detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    int scale = 1;

    for (int octave = 0; octave < octaves_num; ++ octave) {
        for (int scale_level = 1; scale_level < blur_levels_num - 2; ++scale_level) {
            cv::Mat img_1 = DoG[octave][scale_level - 1];
            cv::Mat img_2 = DoG[octave][scale_level];
            cv::Mat img_3 = DoG[octave][scale_level + 1];
            for (int y = 1; y < img_2.rows - 1; ++y) {
                for (int x = 1; x < img_2.cols - 1; ++x) {
                    float center = img_2.at<float>(x, y);

                    if (center >= img_1.at<float>(x - 1, y - 1) &&
                        center >= img_1.at<float>(x - 1, y) &&
                        center >= img_1.at<float>(x - 1, y + 1) &&
                        center >= img_1.at<float>(x, y - 1) &&
                        center >= img_1.at<float>(x, y) &&
                        center >= img_1.at<float>(x, y + 1) &&
                        center >= img_1.at<float>(x + 1, y - 1) &&
                        center >= img_1.at<float>(x + 1, y) &&
                        center >= img_1.at<float>(x + 1, y + 1) &&

                        center >= img_2.at<float>(x - 1, y - 1) &&
                        center >= img_2.at<float>(x - 1, y) &&
                        center >= img_2.at<float>(x - 1, y + 1) &&
                        center >= img_2.at<float>(x, y - 1) &&
                        center >= img_2.at<float>(x, y + 1) &&
                        center >= img_2.at<float>(x + 1, y - 1) &&
                        center >= img_2.at<float>(x + 1, y) &&
                        center >= img_2.at<float>(x + 1, y + 1) &&

                        center >= img_3.at<float>(x - 1, y - 1) &&
                        center >= img_3.at<float>(x - 1, y) &&
                        center >= img_3.at<float>(x - 1, y + 1) &&
                        center >= img_3.at<float>(x, y - 1) &&
                        center >= img_3.at<float>(x, y) &&
                        center >= img_3.at<float>(x, y + 1) &&
                        center >= img_3.at<float>(x + 1, y - 1) &&
                        center >= img_3.at<float>(x + 1, y) &&
                        center >= img_3.at<float>(x + 1, y + 1)) {
                        keypoints.emplace_back(cv::Point2f(x * scale, y * scale), 0.5f, -1, 0, octave);
                    }
                    else if (center <= img_1.at<float>(x - 1, y - 1) &&
                        center <= img_1.at<float>(x - 1, y) &&
                        center <= img_1.at<float>(x - 1, y + 1) &&
                        center <= img_1.at<float>(x, y - 1) &&
                        center <= img_1.at<float>(x, y) &&
                        center <= img_1.at<float>(x, y + 1) &&
                        center <= img_1.at<float>(x + 1, y - 1) &&
                        center <= img_1.at<float>(x + 1, y) &&
                        center <= img_1.at<float>(x + 1, y + 1) &&

                        center <= img_2.at<float>(x - 1, y - 1) &&
                        center <= img_2.at<float>(x - 1, y) &&
                        center <= img_2.at<float>(x - 1, y + 1) &&
                        center <= img_2.at<float>(x, y - 1) &&
                        center <= img_2.at<float>(x, y + 1) &&
                        center <= img_2.at<float>(x + 1, y - 1) &&
                        center <= img_2.at<float>(x + 1, y) &&
                        center <= img_2.at<float>(x + 1, y + 1) &&

                        center <= img_3.at<float>(x - 1, y - 1) &&
                        center <= img_3.at<float>(x - 1, y) &&
                        center <= img_3.at<float>(x - 1, y + 1) &&
                        center <= img_3.at<float>(x, y - 1) &&
                        center <= img_3.at<float>(x, y) &&
                        center <= img_3.at<float>(x, y + 1) &&
                        center <= img_3.at<float>(x + 1, y - 1) &&
                        center <= img_3.at<float>(x + 1, y) &&
                        center <= img_3.at<float>(x + 1, y + 1)) {
                        keypoints.emplace_back(cv::Point2f(x * scale, y * scale), 0.5f, -1, 0, octave);
                    }
                }
            }
        }

        scale *= 2;
    }
}