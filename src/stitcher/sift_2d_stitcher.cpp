#include <cmath>
#include "sift_2d_stitcher.h"


std::shared_ptr<VoxelContainer> SIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        // QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    int overlap = determineOptimalOverlap(scan_1, scan_2);

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z - overlap};

    float* stitchedData = new float[stitchedSize.volume()];

    int offsetVolume = overlap * size_2.x * size_2.y;
    int scanVolume = size_2.x * size_2.y * (size_2.z - overlap);

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    memcpy(stitchedData + size_1.volume(), scan_2.getData() + offsetVolume, scanVolume * sizeof(float));

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, getStitchedRange(scan_1, scan_2));
}


int SIFT2DStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    TiffImage<float> sliceImg_1, sliceImg_2;
    std::vector<std::vector<cv::Mat>> DoG_1, DoG_2;
    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    cv::Mat descriptors_1, descriptors_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches;
    float distancesSum = 0;
    int totalMatches = 0;

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
        descriptors_1.release();
        descriptors_2.release();
        matches.clear();

        buildDoG(slice_1, DoG_1);
        buildDoG(slice_2, DoG_2);

        detect(DoG_1, keypoints_1);
        detect(DoG_2, keypoints_2);

        printf("Finded %lu and %lu candidates to keypoints\n", keypoints_1.size(), keypoints_2.size());

        localize(DoG_1, keypoints_1);
        localize(DoG_2, keypoints_2);

        orient(DoG_1, keypoints_1);
        orient(DoG_2, keypoints_2);

        TiffImage<unsigned char> charSliceImg;
        scan_1.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_1);
        scan_2.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        displayKeypoints(charSliceImg, keypoints_2);

        // calculateDescriptors(DoG_1, keypoints_1, descriptors_1);
        // calculateDescriptors(DoG_2, keypoints_2, descriptors_2);

        matcher.match(descriptors_1, descriptors_2, matches);
        totalMatches += matches.size();

        printf("Matched %lu keypoints\n", matches.size());

        for (int i = 0; i < matches.size(); ++i) {
            auto kp_1 = keypoints_1[matches[i].queryIdx].pt;
            auto kp_2 = keypoints_2[matches[i].trainIdx].pt;
            float distance = (kp_2.y - kp_1.y) / 2;
            printf("Distance: %f\n", distance);
            distancesSum += distance;
        }
    }

    if (totalMatches == 0) {
        printf("No mathces :(\n");
        return 0;
    }

    return distancesSum / totalMatches;
}


void SIFT2DStitcher::testDetection() {
    cv::Mat origImg = cv::imread("../reconstructions/130.png");
    cv::Mat_<float> img;
    cv::cvtColor(origImg, origImg, cv::COLOR_RGB2GRAY);
    origImg.convertTo(img, CV_32F);
    cv::normalize(img, img, 0, 1, cv::NORM_MINMAX);
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    // cv::imshow("Display Keypoints", img);
    // cv::waitKey(0);

    std::vector<std::vector<cv::Mat>> DoG_1;
    std::vector<cv::KeyPoint> keypoints_1;
    cv::Mat descriptors_1;

    buildDoG(img, DoG_1);
    detect(DoG_1, keypoints_1);

    printf("Finded %lu candidates to keypoints\n", keypoints_1.size());

    // cv::Mat rgbSlice;
    // cv::normalize(DoG_1[0][1], rgbSlice, 0, 255, cv::NORM_MINMAX);
    // rgbSlice.convertTo(rgbSlice, CV_8U);
    // cv::drawKeypoints(rgbSlice, keypoints_1, rgbSlice);
    // cv::imshow("Display Keypoints", rgbSlice);
    // cv::waitKey(0);

    // cv::Mat rgbSlice = origImg.clone();
    // cv::drawKeypoints(rgbSlice, keypoints_1, rgbSlice);
    // cv::imshow("Display Keypoints", rgbSlice);
    // cv::imwrite("all_candidates_1.png", rgbSlice);
    // cv::waitKey(0);

    localize(DoG_1, keypoints_1);
    orient(DoG_1, keypoints_1);
    calculateDescriptors(DoG_1, keypoints_1, descriptors_1);
    std::cout << descriptors_1 << std::endl;

    printf("Finded %lu keypoints\n", keypoints_1.size());
    cv::drawKeypoints(origImg, keypoints_1, origImg, cv::Scalar(0, 0, 255), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::imshow("Display Keypoints", origImg);
    // cv::imwrite("all_oriented_1.png", origImg);
    cv::waitKey(0);
    cv::destroyAllWindows();
}


void SIFT2DStitcher::displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints) {
    cv::Mat_<unsigned char> slice(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
    cv::Mat rgbSlice;
    cv::cvtColor(slice, rgbSlice, cv::COLOR_GRAY2RGB);
    cv::drawKeypoints(rgbSlice, keypoints, rgbSlice, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Keypoints", rgbSlice);
    // static int unique_image_id = 0;
    // cv::imwrite("oriented_keypoints_" + std::to_string(unique_image_id++) + ".png", rgbSlice);
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
    const double k = std::pow(2, 1 / static_cast<double>(scale_levels_num));
    
    gaussians.resize(octaves_num);
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
            // cv::normalize(DoG[octave][scale_level], DoG[octave][scale_level], 0, 1, cv::NORM_MINMAX);
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
    const float eigen_ratio = 10;

    std::vector<cv::KeyPoint> true_keypoints;

    for (cv::KeyPoint& kp : keypoints) {
        const int max_attempts = 5;
        int attempt_id = 0;

        float last_kp_val = 0;

        cv::Mat1f grad(3, 1);
        cv::Mat1f hess(3, 3);

        for (attempt_id = 0; attempt_id < max_attempts; ++attempt_id) {
            gradient(DoG, kp, grad);
            hessian(DoG, kp, hess);

            // Estimate position update
            cv::Mat1f inv_hess;
            cv::invert(hess, inv_hess);
            cv::Mat kp_shift = -inv_hess * grad;

            const cv::Mat& kp_img = DoG[kp.octave][kp.class_id];
            last_kp_val = kp_img.at<float>(kp.pt.y, kp.pt.x);

            kp.pt.x += kp_shift.at<float>(0);
            kp.pt.y += kp_shift.at<float>(1);
            kp.class_id += kp_shift.at<float>(2);
            
            if (kp_shift.at<float>(0) < min_shift &&
                kp_shift.at<float>(1) < min_shift &&
                kp_shift.at<float>(2) < min_shift) {
                // Rejecting unstable extrema with low contrast
                float contrast = last_kp_val + 0.5 * (grad.at<float>(0) * kp_shift.at<float>(0) +
                                                      grad.at<float>(1) * kp_shift.at<float>(1) +
                                                      grad.at<float>(2) * kp_shift.at<float>(2));

                if (contrast * scale_levels_num < min_contrast) {
                    printf("Low contrast keypoint discarded\n");
                    break;
                }

                // Eliminating edge responses
                float hess_det = hess.at<float>(0, 0) * hess.at<float>(1, 1) - hess.at<float>(0, 1) * hess.at<float>(1, 0);
                float hess_trace = hess.at<float>(0, 0) + hess.at<float>(1, 1);

                if (hess_det <= 0 || eigen_ratio * hess_trace * hess_trace >= (eigen_ratio + 1) * (eigen_ratio + 1) * hess_det) {
                    printf("High edge response keypoint discarded\n");
                    break;
                }

                // (kp.octave + 1) ???
                kp.size = sigma * std::pow(2, kp.class_id / static_cast<float>(scale_levels_num)) * std::pow(2, kp.octave + 1);

                // Convert keypoint position to original scale space
                kp.pt *= std::pow(2, kp.octave);

                true_keypoints.push_back(kp);
                break;
            }

            // Check if keypoint is in layer bounds
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


float SIFT2DStitcher::parabolicInterpolation(float y1, float y2, float y3) {
    // Assume that x1 = -1, x2 = 0, x3 = 1
    float a = y2 - (y1 + y3) / 2;
    float b = (y3 - y1) / 4;
    return b / a;
}


void SIFT2DStitcher::orient(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    const float scale_factor = 1.5;
    const int hist_bins_num = 36;
    const int kps_num = keypoints.size();

    cv::Mat HoG = cv::Mat1f::zeros(kps_num, hist_bins_num);

    for (int kp_id = 0; kp_id < kps_num; ++kp_id) {
        cv::KeyPoint& kp = keypoints[kp_id];

        // Calculate HoG
        const float sigma = scale_factor * kp.size / std::pow(2, kp.octave + 1);
        const float radius = 3 * sigma;
        const float weight_factor = -1 / (2 * sigma * sigma);
        
        cv::Point center = kp.pt / std::pow(2, kp.octave);

        const cv::Mat& img = gaussians[kp.octave][kp.class_id];

        for (int y = center.y - radius; y <= center.y + radius; ++y) {
            if (y <= 0 || y >= img.rows - 1) {
                continue;
            }

            for (int x = center.x - radius; x <= center.x + radius; ++x) {
                if (x <= 0 || x >= img.cols - 1) {
                    continue;
                }

                float dx = img.at<float>(y, x + 1) - img.at<float>(y, x - 1);
                float dy = img.at<float>(y + 1, x) - img.at<float>(y - 1, x);
                float magnitude = std::sqrt(dx * dx + dy * dy);
                float orientation = std::atan2(dy, dx) * 180.0f / M_PI + 180;
                
                int x_loc = x - center.x;
                int y_loc = y - center.y;
                float weight = std::exp(weight_factor * (x_loc * x_loc + y_loc * y_loc));

                // Add orientation to histogram weighted with gaussian
                int hist_id = orientation * hist_bins_num / 360.0f;
                HoG.at<float>(kp_id, hist_id) += weight * magnitude;
            }
        }

        // Find max peak
        int max_bin = 0;
        float max_val = 0;

        for (int bin = 0; bin < hist_bins_num; ++bin) {
            if (HoG.at<float>(kp_id, bin) > max_val) {
                max_val = HoG.at<float>(kp_id, bin);
                max_bin = bin;
            }
        }

        // Interpolate peak with its two neighbours
        float left_val = HoG.at<float>(kp_id, (max_bin - 1 + hist_bins_num) % hist_bins_num);
        float right_val = HoG.at<float>(kp_id, (max_bin + 1) % hist_bins_num);
        kp.angle = (parabolicInterpolation(left_val, max_val, right_val) + max_bin) * 10;

        // TEMPORARY FOR VISUALIZATION
        kp.size *= 3;

        // Find other peaks
        float bin_threshold = max_val * 0.8;

        for (int bin = 0; bin < hist_bins_num; ++bin) {
            float val = HoG.at<float>(kp_id, bin);

            if (bin != max_bin && val > bin_threshold) {
                // Interpolate peak with its two neighbours
                float left_val = HoG.at<float>(kp_id, (bin - 1 + hist_bins_num) % hist_bins_num);
                float right_val = HoG.at<float>(kp_id, (bin + 1) % hist_bins_num);
                float angle = (parabolicInterpolation(left_val, val, right_val) + bin) * 10;

                // Create new keypoint on the same place with other orientation
                keypoints.emplace_back(kp.pt, kp.size, angle, kp.response, kp.octave, kp.class_id);
                printf("Angle: %f\n", angle);
            }
        }
    }
}


void SIFT2DStitcher::calculateDescriptors(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat descriptors) {
    const int window_width = 4;
    const int region_width = 4;
    const int radius = window_width * region_width / 2;
    const int hist_bins_num = 8;
    const int kps_num = keypoints.size();

    descriptors = cv::Mat1f::zeros(kps_num, window_width * window_width * hist_bins_num);

    for (int kp_id = 0; kp_id < kps_num; ++kp_id) {
        cv::KeyPoint& kp = keypoints[kp_id];

        const float sigma = window_width / 2;
        const float weight_factor = -1 / (2 * sigma * sigma); 
        // const float angle = kp.angle * M_PI / 180.0f;
        // const float cos_angle = std::cos(angle);
        // const float sin_angle = std::sin(angle);
        
        cv::Point center = kp.pt / std::pow(2, kp.octave);

        const cv::Mat& img = gaussians[kp.octave][kp.class_id];

        for (int y = center.y - radius; y <= center.y + radius; ++y) {
            if (y <= 0 || y >= img.rows - 1) {
                continue;
            }

            for (int x = center.x - radius; x <= center.x + radius; ++x) {
                if (x <= 0 || x >= img.cols - 1) {
                    continue;
                }

                float dx = img.at<float>(y, x + 1) - img.at<float>(y, x - 1);
                float dy = img.at<float>(y + 1, x) - img.at<float>(y - 1, x);
                float magnitude = std::sqrt(dx * dx + dy * dy);
                float orientation = std::atan2(dy, dx) * 180.0f / M_PI;
                float weight = std::exp(weight_factor * (x * x + y * y));

                // Add orientation to histogram weighted with gaussian
                int hist_row = y / region_width;
                int hist_col = x / region_width;
                int hist_id = (hist_row * window_width + hist_col) * hist_bins_num + orientation * hist_bins_num / 360.0f;
                descriptors.at<float>(kp_id, hist_id) += weight * magnitude;
            }
        }
    }
}
