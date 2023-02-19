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

    cv::Mat rgbSlice;
    cv::normalize(DoG_1[0][1], rgbSlice, 0, 255, cv::NORM_MINMAX);
    rgbSlice.convertTo(rgbSlice, CV_8U);
    cv::drawKeypoints(rgbSlice, keypoints_1, rgbSlice);
    cv::imshow("Display Keypoints", rgbSlice);
    cv::waitKey(0);

    printf("Finded %lu candidates to keypoints\n", keypoints_1.size());
    localize(DoG_1, keypoints_1);

    printf("Finded %lu keypoints\n", keypoints_1.size());
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
            cv::normalize(DoG[octave][scale_level], DoG[octave][scale_level], 0, 1, cv::NORM_MINMAX);
        }
    }
}


void SIFT2DStitcher::detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    int scale = 1;

    for (int octave = 0; octave < octaves_num; ++ octave) {
        for (int scale_level = 1; scale_level < blur_levels_num - 2; ++scale_level) {
            cv::Mat img_0 = DoG[octave][scale_level - 1];
            cv::Mat img_1 = DoG[octave][scale_level];
            cv::Mat img_2 = DoG[octave][scale_level + 1];
            for (int x = 1; x < img_1.rows - 1; ++x) {
                for (int y = 1; y < img_1.cols - 1; ++y) {
                    float center = img_1.at<float>(x, y);

                    if (center >= img_0.at<float>(x - 1, y - 1) &&
                        center >= img_0.at<float>(x - 1, y) &&
                        center >= img_0.at<float>(x - 1, y + 1) &&
                        center >= img_0.at<float>(x, y - 1) &&
                        center >= img_0.at<float>(x, y) &&
                        center >= img_0.at<float>(x, y + 1) &&
                        center >= img_0.at<float>(x + 1, y - 1) &&
                        center >= img_0.at<float>(x + 1, y) &&
                        center >= img_0.at<float>(x + 1, y + 1) &&

                        center >= img_1.at<float>(x - 1, y - 1) &&
                        center >= img_1.at<float>(x - 1, y) &&
                        center >= img_1.at<float>(x - 1, y + 1) &&
                        center >= img_1.at<float>(x, y - 1) &&
                        center >= img_1.at<float>(x, y + 1) &&
                        center >= img_1.at<float>(x + 1, y - 1) &&
                        center >= img_1.at<float>(x + 1, y) &&
                        center >= img_1.at<float>(x + 1, y + 1) &&

                        center >= img_2.at<float>(x - 1, y - 1) &&
                        center >= img_2.at<float>(x - 1, y) &&
                        center >= img_2.at<float>(x - 1, y + 1) &&
                        center >= img_2.at<float>(x, y - 1) &&
                        center >= img_2.at<float>(x, y) &&
                        center >= img_2.at<float>(x, y + 1) &&
                        center >= img_2.at<float>(x + 1, y - 1) &&
                        center >= img_2.at<float>(x + 1, y) &&
                        center >= img_2.at<float>(x + 1, y + 1)) {
                        keypoints.emplace_back(cv::Point2f(y, x), 0.5f, -1, 0, octave, scale_level);
                    }
                    else if (center <= img_0.at<float>(x - 1, y - 1) &&
                        center <= img_0.at<float>(x - 1, y) &&
                        center <= img_0.at<float>(x - 1, y + 1) &&
                        center <= img_0.at<float>(x, y - 1) &&
                        center <= img_0.at<float>(x, y) &&
                        center <= img_0.at<float>(x, y + 1) &&
                        center <= img_0.at<float>(x + 1, y - 1) &&
                        center <= img_0.at<float>(x + 1, y) &&
                        center <= img_0.at<float>(x + 1, y + 1) &&

                        center <= img_1.at<float>(x - 1, y - 1) &&
                        center <= img_1.at<float>(x - 1, y) &&
                        center <= img_1.at<float>(x - 1, y + 1) &&
                        center <= img_1.at<float>(x, y - 1) &&
                        center <= img_1.at<float>(x, y + 1) &&
                        center <= img_1.at<float>(x + 1, y - 1) &&
                        center <= img_1.at<float>(x + 1, y) &&
                        center <= img_1.at<float>(x + 1, y + 1) &&

                        center <= img_2.at<float>(x - 1, y - 1) &&
                        center <= img_2.at<float>(x - 1, y) &&
                        center <= img_2.at<float>(x - 1, y + 1) &&
                        center <= img_2.at<float>(x, y - 1) &&
                        center <= img_2.at<float>(x, y) &&
                        center <= img_2.at<float>(x, y + 1) &&
                        center <= img_2.at<float>(x + 1, y - 1) &&
                        center <= img_2.at<float>(x + 1, y) &&
                        center <= img_2.at<float>(x + 1, y + 1)) {
                        keypoints.emplace_back(cv::Point2f(y, x), 0.5f, -1, 0, octave, scale_level);
                    }
                }
            }
        }

        scale *= 2;
    }
}


void SIFT2DStitcher::gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result) {
    const cv::Mat& img_0 = DoG[kp.octave][kp.class_id - 1];
    const cv::Mat& img_1 = DoG[kp.octave][kp.class_id];
    const cv::Mat& img_2 = DoG[kp.octave][kp.class_id + 1];

    result.at<float>(0) = 0.5 * (img_1.at<float>(kp.pt.y, kp.pt.x + 1) - img_1.at<float>(kp.pt.y, kp.pt.x - 1));
    result.at<float>(1) = 0.5 * (img_1.at<float>(kp.pt.y + 1, kp.pt.x) - img_1.at<float>(kp.pt.y - 1, kp.pt.x));
    result.at<float>(2) = 0.5 * (img_2.at<float>(kp.pt.y, kp.pt.x) - img_0.at<float>(kp.pt.y, kp.pt.x));
}


void SIFT2DStitcher::hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result) {
    const cv::Mat& img_0 = DoG[kp.octave][kp.class_id - 1];
    const cv::Mat& img_1 = DoG[kp.octave][kp.class_id];
    const cv::Mat& img_2 = DoG[kp.octave][kp.class_id + 1];

    float center = img_1.at<float>(kp.pt.y, kp.pt.x);

    result.at<float>(0, 0) = img_1.at<float>(kp.pt.y, kp.pt.x + 1) + img_1.at<float>(kp.pt.y, kp.pt.x - 1) - 2 * center;
    result.at<float>(1, 1) = img_1.at<float>(kp.pt.y + 1, kp.pt.x) + img_1.at<float>(kp.pt.y - 1, kp.pt.x) - 2 * center;
    result.at<float>(2, 2) = img_2.at<float>(kp.pt.y, kp.pt.x) + img_0.at<float>(kp.pt.y, kp.pt.x) - 2 * center;

    result.at<float>(0, 1) = 0.25 * (img_1.at<float>(kp.pt.y + 1, kp.pt.x + 1) -
                                     img_1.at<float>(kp.pt.y + 1, kp.pt.x - 1) -
                                     img_1.at<float>(kp.pt.y - 1, kp.pt.x + 1) +
                                     img_1.at<float>(kp.pt.y - 1, kp.pt.x - 1));

    result.at<float>(1, 0) = result.at<float>(0, 1);

    result.at<float>(0, 2) = 0.25 * (img_2.at<float>(kp.pt.y, kp.pt.x + 1) -
                                     img_2.at<float>(kp.pt.y, kp.pt.x - 1) -
                                     img_0.at<float>(kp.pt.y, kp.pt.x + 1) +
                                     img_0.at<float>(kp.pt.y, kp.pt.x - 1));

    result.at<float>(2, 0) = result.at<float>(0, 2);

    result.at<float>(1, 2) = 0.25 * (img_2.at<float>(kp.pt.y + 1, kp.pt.x) -
                                     img_2.at<float>(kp.pt.y - 1, kp.pt.x) -
                                     img_0.at<float>(kp.pt.y + 1, kp.pt.x) +
                                     img_0.at<float>(kp.pt.y - 1, kp.pt.x));
    
    result.at<float>(2, 1) = result.at<float>(1, 2);
}


void SIFT2DStitcher::localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    const float min_shift = 0.5;
    const float min_contrast = 0.03;

    std::vector<cv::KeyPoint> true_keypoints;

    for (cv::KeyPoint& kp : keypoints) {
        const int max_attempts = 5;
        int attempt_id = 0;

        cv::Mat1f grad(3, 1);
        cv::Mat1f hess(3, 3);

        for (attempt_id = 0; attempt_id < max_attempts; ++attempt_id) {
            gradient(DoG, kp, grad);
            hessian(DoG, kp, hess);

            cv::Mat1f inv_hess;
            cv::invert(hess, inv_hess);
            cv::Mat kp_shift = -inv_hess * grad;

            kp.pt.x += kp_shift.at<float>(0);
            kp.pt.y += kp_shift.at<float>(1);
            kp.class_id += kp_shift.at<float>(2);
            
            if (kp_shift.at<float>(0) < min_shift &&
                kp_shift.at<float>(1) < min_shift &&
                kp_shift.at<float>(2) < min_shift) {
                float contrast = std::abs(kp_shift.at<float>(0) * kp_shift.at<float>(0) +
                                          kp_shift.at<float>(1) * kp_shift.at<float>(1) +
                                          kp_shift.at<float>(2) * kp_shift.at<float>(2));

                if (contrast < min_contrast) {
                    printf("Low contrast keypoint discarded\n");
                    break;
                }

                true_keypoints.push_back(kp);
                break;
            }

            const cv::Mat& kp_img = DoG[kp.octave][kp.class_id];

            if (1 > kp.pt.x ||
                kp.pt.x >= kp_img.cols - 1 ||
                1 > kp.pt.y ||
                kp.pt.y >= kp_img.rows - 1 ||
                1 > kp.class_id ||
                kp.class_id >= blur_levels_num - 2) {
                printf("Keypoint moved outside of image\n");
                break;
            }
        }

        if (attempt_id == max_attempts) {
            printf("Maximum number attempts to move keypoint exceeded\n");
        }
    }

    keypoints = std::move(true_keypoints);
}