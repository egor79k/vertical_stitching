#include <cmath>
#include "sift_3d_stitcher.h"


void SIFT3DStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    // VoxelContainer::Vector3 size_1 = scan_1.getSize();
    // VoxelContainer::Vector3 size_2 = scan_2.getSize();

    std::vector<std::vector<VoxelContainer>> scanGaussians_1;
    std::vector<std::vector<VoxelContainer>> scanDoGs_1;
    std::vector<std::vector<cv::Mat>> gaussians_1;
    std::vector<std::vector<cv::Mat>> DoG_1;
    std::vector<cv::KeyPoint> keypoints_1;
    cv::Mat descriptors_1;

    buildDoG(scan_1, scanGaussians_1, scanDoGs_1);

    // cv::Mat descriptors_1;

    // slicesGaussians_1.resize(planes.size());
    // slicesDoGs_1.resize(planes.size());
    // keypoints_1.resize(planes.size());

    TiffImage<float> sliceImg;

    gaussians_1.resize(octavesNum);
    DoG_1.resize(octavesNum);

    for (int octave = 0; octave < octavesNum; ++octave) {
        gaussians_1[octave].resize(blurLevelsNum);
        DoG_1[octave].resize(blurLevelsNum - 1);
    }

    for (int plane : planes) {
        // DoG_1.clear();
        // DoG_2.clear();
        // keypoints_1.clear();
        // keypoints_2.clear();
        // descriptors_1.release();
        // descriptors_2.release();
        // matches.clear();

        for (int octave = 0; octave < octavesNum; ++octave) {
            for (int scale_level = 0; scale_level < blurLevelsNum; ++scale_level) {
                VoxelContainer::Vector3 size = scanGaussians_1[octave][scale_level].getSize();
                scanGaussians_1[octave][scale_level].getSlice<float>(sliceImg, plane, size.x / 2, false);
                gaussians_1[octave][scale_level] = cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
            }

            for (int scale_level = 0; scale_level < blurLevelsNum - 1; ++scale_level) {
                VoxelContainer::Vector3 size = scanDoGs_1[octave][scale_level].getSize();
                scanDoGs_1[octave][scale_level].getSlice<float>(sliceImg, plane, size.x / 2, false);
                DoG_1[octave][scale_level] = cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
            }
        }


        // int slice_id = size_1.x / 2;

        // scan_1.getSlice<float>(sliceImg_1, plane, slice_id, false);
        // scan_2.getSlice<float>(sliceImg_2, plane, slice_id, false);

        // cv::Mat_<float> slice_1(size_1.z, size_1.y, sliceImg_1.getData());
        // cv::Mat_<float> slice_2(size_2.z, size_2.y, sliceImg_2.getData());

        detect(DoG_1, keypoints_1);
        // detect(DoG_2, keypoints_2);
        printf("Finded %lu candidates to keypoints\n", keypoints_1.size());

        localize(DoG_1, keypoints_1);
        // localize(DoG_2, keypoints_2);

        orient(gaussians_1, DoG_1, keypoints_1);
        // orient(gaussians_2, DoG_2, keypoints_2);

        TiffImage<unsigned char> charSliceImg;
        scan_1.getSlice<unsigned char>(charSliceImg, plane, scan_1.getSize().x / 2, true);
        displayKeypoints(charSliceImg, keypoints_1);
        // scan_2.getSlice<unsigned char>(charSliceImg, plane, slice_id, true);
        // displayKeypoints(charSliceImg, keypoints_2);
        // calculateDescriptors(gaussians_1, DoG_1, keypoints_1, descriptors_1);
        // calculateDescriptors(gaussians_2, DoG_2, keypoints_2, descriptors_2);

        // matcher.match(descriptors_1, descriptors_2, matches);
        // totalMatches += matches.size();

        // printf("Matched %lu keypoints\n", matches.size());

        // for (int i = 0; i < matches.size(); ++i) {
        //     auto kp_1 = keypoints_1[matches[i].queryIdx].pt;
        //     auto kp_2 = keypoints_2[matches[i].trainIdx].pt;
        //     float distance = (kp_2.y - kp_1.y) / 2;
        //     printf("Distance: %f\n", distance);
        //     distancesSum += distance;
        // }
    }

    // if (totalMatches == 0) {
    //     printf("No mathces :(\n");
    //     return 0;
    // }

    // return distancesSum / totalMatches;
    scan_2.setEstStitchParams({0, 0, static_cast<int>(scan_1.getSize().z)});
}


void SIFT3DStitcher::displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints) {
    cv::Mat_<unsigned char> slice(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
    cv::Mat rgbSlice;
    cv::cvtColor(slice, rgbSlice, cv::COLOR_GRAY2RGB);
    cv::drawKeypoints(rgbSlice, keypoints, rgbSlice, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::resize(rgbSlice, rgbSlice, {0, 0}, 4, 4);
    cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Keypoints", rgbSlice);
    // static int unique_image_id = 0;
    // cv::imwrite("oriented_keypoints_" + std::to_string(unique_image_id++) + ".png", rgbSlice);
    // cv::waitKey(0);
    int key = -1;
    while (key != 'q') key = cv::waitKeyEx(100);
    cv::destroyAllWindows();
}


void SIFT3DStitcher::displaySlice(const VoxelContainer& src) {
    TiffImage<float> sliceImg;
    src.getSlice<float>(sliceImg, 0, src.getSize().x / 2, false);
    cv::Mat_<float> slice(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData());
    cv::normalize(slice, slice, 0, 1, cv::NORM_MINMAX);
    // cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display slice", slice);
    // static int unique_image_id = 0;
    // cv::imwrite("oriented_keypoints_" + std::to_string(unique_image_id++) + ".png", rgbSlice);
    cv::waitKey(0);
    cv::destroyAllWindows();
}


void SIFT3DStitcher::gaussianBlur(const VoxelContainer& src, VoxelContainer& dst, const double sigma) {
    int radius = 2 * sigma;
    size_t gSize = 2 * radius + 1;
    // std::cout << "RS: " << radius << ' ' << sigma << std::endl;
    VoxelContainer gaussian({gSize, gSize, gSize}, {0, 1});
    float sum = 0;
    
    for (int z = -radius; z <= radius; ++z) {
        for (int y = -radius; y <= radius; ++y) {
            for (int x = -radius; x <= radius; ++x) {
                float val = std::exp(-(x * x + y * y + z * z) / (2 * sigma * sigma));
                gaussian.at(x + radius, y + radius, z + radius) = val;
                sum += val;
            }
        }
    }

    VoxelContainer::Vector3 size = src.getSize();
    // std::cout << "SIZE: " << size.x << ' ' << size.y << ' ' << size.z << ' ' << size.volume() << std::endl;
    dst.create(size, src.getRange());

    // displaySlice(gaussian);

    for (int sz = 0; sz < size.z; ++sz) {
        std::cout << sz << " of " << size.z << std::endl;
        for (int sy = 0; sy < size.y; ++sy) {
            for (int sx = 0; sx < size.x; ++sx) {
                for (int z = -radius; z <= radius; ++z) {
                    int lz = sz + z;
                    if (lz < 0 || lz >= size.z) {
                        continue;
                    }
                    for (int y = -radius; y <= radius; ++y) {
                        int ly = sy + y;
                        if (ly < 0 || ly >= size.y) {
                            continue;
                        }
                        for (int x = -radius; x <= radius; ++x) {
                            int lx = sx + x;
                            if (lx < 0 || lx >= size.x) {
                                continue;
                            }
                            dst.at(sx, sy, sz) += src.at(lx, ly, lz) * gaussian.at(x + radius, y + radius, z + radius) / sum;
                        }
                    }
                }
            }
        }
    }

    // displaySlice(dst);
}


void SIFT3DStitcher::compressTwice(const VoxelContainer& src, VoxelContainer& dst) {
    VoxelContainer::Vector3 srcSize = src.getSize();
    VoxelContainer::Vector3 dstSize = {(srcSize.x + 1) / 2, (srcSize.y + 1) / 2, (srcSize.z + 1) / 2};
    dst.create(dstSize, src.getRange());

    for (int z = 0; z < dstSize.z; ++z) {
        for (int y = 0; y < dstSize.y; ++y) {
            for (int x = 0; x < dstSize.x; ++x) {
                dst.at(x, y, z) = src.at(x * 2, y * 2, z * 2);
            }
        }
    }
}


void SIFT3DStitcher::buildDoG(const VoxelContainer& vol, std::vector<std::vector<VoxelContainer>>& gaussians, std::vector<std::vector<VoxelContainer>>& DoG) {
    const double k = std::pow(2, 1 / static_cast<double>(scaleLevelsNum));
    
    gaussians.resize(octavesNum);
    DoG.resize(octavesNum);

    for (int i = 0; i < octavesNum; ++i) {
        gaussians[i].resize(blurLevelsNum);
        DoG[i].resize(blurLevelsNum - 1);
    }

    gaussianBlur(vol, gaussians[0][0], sigma);

    for (int octave = 0; octave < octavesNum; ++octave) {
        for (int scale_level = 1; scale_level < blurLevelsNum; ++scale_level) {
            std::cout << "octave: " << octave << " scale: " << scale_level << std::endl;
            double kernel = sigma * std::pow(2, octave) * std::pow(k, scale_level);
            gaussianBlur(gaussians[octave][scale_level - 1], gaussians[octave][scale_level], kernel);
        }

        if (octave < octavesNum - 1) {
            compressTwice(gaussians[octave][scaleLevelsNum], gaussians[octave + 1][0]);
        }
    }

    for (int octave = 0; octave < octavesNum; ++ octave) {
        for (int scale_level = 0; scale_level < blurLevelsNum - 1; ++scale_level) {
            // DoG[octave][scale_level] = gaussians[octave][scale_level] - gaussians[octave][scale_level + 1];
            substract(gaussians[octave][scale_level], gaussians[octave][scale_level + 1], DoG[octave][scale_level]);
            // displaySlice(DoG[octave][scale_level]);
        }
    }
}


void SIFT3DStitcher::detect(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
    int scale = 1;

    for (int octave = 0; octave < octavesNum; ++ octave) {
        for (int scale_level = 1; scale_level < blurLevelsNum - 2; ++scale_level) {
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


void SIFT3DStitcher::gradient(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result) {
    const cv::Mat& img_0 = DoG[kp.octave][kp.class_id - 1];
    const cv::Mat& img_1 = DoG[kp.octave][kp.class_id];
    const cv::Mat& img_2 = DoG[kp.octave][kp.class_id + 1];

    result.at<float>(0) = 0.5 * (img_1.at<float>(kp.pt.y, kp.pt.x + 1) - img_1.at<float>(kp.pt.y, kp.pt.x - 1));
    result.at<float>(1) = 0.5 * (img_1.at<float>(kp.pt.y + 1, kp.pt.x) - img_1.at<float>(kp.pt.y - 1, kp.pt.x));
    result.at<float>(2) = 0.5 * (img_2.at<float>(kp.pt.y, kp.pt.x) - img_0.at<float>(kp.pt.y, kp.pt.x));
}


void SIFT3DStitcher::hessian(const std::vector<std::vector<cv::Mat>>& DoG, const cv::KeyPoint& kp, cv::Mat1f& result) {
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


void SIFT3DStitcher::localize(const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
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

                if (contrast * scaleLevelsNum < min_contrast) {
                    // printf("Low contrast keypoint discarded\n");
                    break;
                }

                // Eliminating edge responses
                float hess_det = hess.at<float>(0, 0) * hess.at<float>(1, 1) - hess.at<float>(0, 1) * hess.at<float>(1, 0);
                float hess_trace = hess.at<float>(0, 0) + hess.at<float>(1, 1);

                if (hess_det <= 0 || eigen_ratio * hess_trace * hess_trace >= (eigen_ratio + 1) * (eigen_ratio + 1) * hess_det) {
                    // printf("High edge response keypoint discarded\n");
                    break;
                }

                // (kp.octave + 1) ???
                kp.size = sigma * std::pow(2, kp.class_id / static_cast<float>(scaleLevelsNum)) * std::pow(2, kp.octave + 1);

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
                kp.class_id >= blurLevelsNum - 2) {
                // printf("Keypoint moved outside of image\n");
                break;
            }
        }

        if (attempt_id == max_attempts) {
            // printf("Maximum number attempts to move keypoint exceeded\n");
        }
    }

    keypoints = std::move(true_keypoints);
}


float SIFT3DStitcher::parabolicInterpolation(float y1, float y2, float y3) {
    // Assume that x1 = -1, x2 = 0, x3 = 1
    float a = y2 - (y1 + y3) / 2;
    float b = (y3 - y1) / 4;
    return b / a;
}


void SIFT3DStitcher::orient(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints) {
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

        if (kp.angle < 0) {
            kp.angle += 360;
        }

        // TEMPORARY FOR VISUALIZATION
        kp.size *= 3;

        // Find other peaks
        float bin_threshold = max_val * 0.8;

        for (int bin = 0; bin < hist_bins_num; ++bin) {
            float val = HoG.at<float>(kp_id, bin);

            if (bin != max_bin && val > bin_threshold) {
                // Interpolate peak with its two neighbours
                left_val = HoG.at<float>(kp_id, (bin - 1 + hist_bins_num) % hist_bins_num);
                right_val = HoG.at<float>(kp_id, (bin + 1) % hist_bins_num);
                float angle = (parabolicInterpolation(left_val, val, right_val) + bin) * 10;

                if (angle < 0) {
                    angle += 360;
                }

                // Create new keypoint on the same place with other orientation
                keypoints.emplace_back(kp.pt, kp.size, angle, kp.response, kp.octave, kp.class_id);
            }
        }
    }
}


void SIFT3DStitcher::calculateDescriptors(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat descriptors) {
    const int window_width = 4;
    const int region_width = 4;
    const int radius = window_width * region_width / 2;
    const int hist_bins_num = 8;
    const int degrees_per_bin = 360 / hist_bins_num;
    const int kps_num = keypoints.size();
    const float descriptor_threshold = 0.2;

    descriptors = cv::Mat1f::zeros(kps_num, window_width * window_width * hist_bins_num);

    for (int kp_id = 0; kp_id < kps_num; ++kp_id) {
        cv::KeyPoint& kp = keypoints[kp_id];

        const float sigma = window_width / 2;
        const float weight_factor = -1 / (2 * sigma * sigma); 
        // const float angle = kp.angle * M_PI / 180.0f;
        // const float cos_angle = std::cos(angle);
        // const float sin_angle = std::sin(angle);
        
        float scale = std::pow(2, kp.octave);
        cv::Point center(std::round(kp.pt.x / scale), std::round(kp.pt.y / scale));

        const cv::Mat& img = gaussians[kp.octave][kp.class_id];

        for (int y = center.y - radius; y < center.y + radius; ++y) {
            if (y <= 0 || y >= img.rows - 1) {
                continue;
            }

            for (int x = center.x - radius; x < center.x + radius; ++x) {
                if (x <= 0 || x >= img.cols - 1) {
                    continue;
                }

                float dx = img.at<float>(y, x + 1) - img.at<float>(y, x - 1);
                float dy = img.at<float>(y + 1, x) - img.at<float>(y - 1, x);
                float magnitude = std::sqrt(dx * dx + dy * dy);
                float orientation = std::atan2(dy, dx) * 180.0f / M_PI;

                // Rotate on keypoint orientation for rotational invariance
                orientation = std::fmod(orientation + kp.angle, 360);

                int x_loc = x - center.x;
                int y_loc = y - center.y;
                float gauss_weight = std::exp(weight_factor * (x_loc * x_loc + y_loc * y_loc));

                // Trilinear interpolation coefficients
                float x_weight = 1 - std::abs(x_loc % region_width - region_width / 2) / static_cast<float>(region_width);
                float y_weight = 1 - std::abs(y_loc % region_width - region_width / 2) / static_cast<float>(region_width);
                float a_weight = 1 - std::abs(std::fmod(orientation, degrees_per_bin) - degrees_per_bin / 2) / degrees_per_bin;

                // Add weighted orientation to histogram
                int hist_row = (y_loc + radius) / region_width;
                int hist_col = (x_loc + radius) / region_width;
                int hist_id = (hist_row * window_width + hist_col) * hist_bins_num + orientation * hist_bins_num / 360.0f;
                descriptors.at<float>(kp_id, hist_id) += gauss_weight * x_weight * y_weight * a_weight * magnitude;
            }
        }

        // Descriptor normalization with truncating big values
        cv::Mat descriptor = descriptors.row(kp_id);
        cv::normalize(descriptor, descriptor);
        cv::threshold(descriptor, descriptor, descriptor_threshold, descriptor_threshold, cv::THRESH_TRUNC);
        cv::normalize(descriptor, descriptor);

        // for (int i = 0; i < descriptor.cols; ++i) {
        //     printf("%f ", descriptor.at<float>(i));
        // }
        // printf("]\n\n[");
    }
}
