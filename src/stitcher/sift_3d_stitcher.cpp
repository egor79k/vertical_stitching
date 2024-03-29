#include <cmath>
#include "sift_3d_stitcher.h"


float SIFT3DStitcher::getMedian(std::vector<float>& array) {
    int size = array.size();
    
    if (size == 0) {
        return 1;
    }
    
    if (size < 3) {
        float sum = 0;
        for (auto val : array) {
            sum += val;
        }
        return sum / size;
    }

    std::sort(array.begin(), array.end());

    std::cout << std::endl;

    return array[size / 2];
}


void SIFT3DStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    std::vector<float> offsetsX;
    std::vector<float> offsetsY;
    std::vector<float> offsetsZ;

    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    // Calculate optimal params
    int size = std::max(size_1.x, size_1.y);
    octavesNum = std::log2(size / 16);

    if (size < 512) {
        // sigma = 1.4 * size / 512 + 0.2;
        sigma = 0.7;
    }

    const int refOffsetZ = scan_2.getRefStitchParams().offsetZ;
    int maxOverlap = size_2.z / 2;

    if (refOffsetZ > 0) {
        maxOverlap = size_1.z - refOffsetZ;
        int maxDeviation = std::max(5, maxOverlap / 5);
        maxOverlap += maxDeviation;
    }

    std::vector<std::vector<VoxelContainer>> scanGaussians_1, scanGaussians_2;
    std::vector<std::vector<VoxelContainer>> scanDoGs_1, scanDoGs_2;
    std::vector<std::vector<cv::Mat>> gaussians_1, gaussians_2;
    std::vector<std::vector<cv::Mat>> DoG_1, DoG_2;
    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    std::vector<cv::DMatch> matches;
    cv::Mat descriptors_1, descriptors_2;
    cv::BFMatcher matcher;

    const int start_1 = size_1.z - maxOverlap;
    const int end_1 = size_1.z;
    const int start_2 = 0;
    const int end_2 = maxOverlap;

    buildDoG(scan_1, scanGaussians_1, scanDoGs_1, start_1, end_1);
    buildDoG(scan_2, scanGaussians_2, scanDoGs_2, start_2, end_2);

    TiffImage<float> sliceImg;

    for (auto plane : planes) {
        DoG_1.clear();
        DoG_2.clear();
        keypoints_1.clear();
        keypoints_2.clear();
        descriptors_1.release();
        descriptors_2.release();
        matches.clear();

        gaussians_1.resize(octavesNum);
        gaussians_2.resize(octavesNum);
        DoG_1.resize(octavesNum);
        DoG_2.resize(octavesNum);

        for (int octave = 0; octave < octavesNum; ++octave) {
            gaussians_1[octave].resize(blurLevelsNum);
            gaussians_2[octave].resize(blurLevelsNum);
            DoG_1[octave].resize(blurLevelsNum - 1);
            DoG_2[octave].resize(blurLevelsNum - 1);
        }

        for (int octave = 0; octave < octavesNum; ++octave) {
            for (int scale_level = 0; scale_level < blurLevelsNum; ++scale_level) {
                VoxelContainer::Vector3 gSize_1 = scanGaussians_1[octave][scale_level].getSize();
                scanGaussians_1[octave][scale_level].getSlice<float>(sliceImg, plane.first, gSize_1.x * plane.second, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(gaussians_1[octave][scale_level]);
                
                VoxelContainer::Vector3 gSize_2 = scanGaussians_2[octave][scale_level].getSize();
                scanGaussians_2[octave][scale_level].getSlice<float>(sliceImg, plane.first, gSize_2.x * plane.second, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(gaussians_2[octave][scale_level]);
            }

            for (int scale_level = 0; scale_level < blurLevelsNum - 1; ++scale_level) {
                VoxelContainer::Vector3 dSize_1 = scanDoGs_1[octave][scale_level].getSize();
                scanDoGs_1[octave][scale_level].getSlice<float>(sliceImg, plane.first, dSize_1.x * plane.second, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(DoG_1[octave][scale_level]);
                
                VoxelContainer::Vector3 dSize_2 = scanDoGs_2[octave][scale_level].getSize();
                scanDoGs_2[octave][scale_level].getSlice<float>(sliceImg, plane.first, dSize_2.x * plane.second, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(DoG_2[octave][scale_level]);
            }
        }

        detect(DoG_1, keypoints_1);
        detect(DoG_2, keypoints_2);

        printf("Finded %lu and %lu candidates to keypoints\n", keypoints_1.size(), keypoints_2.size());

        localize(DoG_1, keypoints_1);
        localize(DoG_2, keypoints_2);

        printf("Localized %lu and %lu keypoints\n", keypoints_1.size(), keypoints_2.size());

        orient(gaussians_1, DoG_1, keypoints_1);
        orient(gaussians_2, DoG_2, keypoints_2);

        printf("Oriented %lu and %lu keypoints\n", keypoints_1.size(), keypoints_2.size());

        // TiffImage<unsigned char> charSliceImg_1, charSliceImg_2;
        // scan_1.getSlice<unsigned char>(charSliceImg_1, plane.first, size_1.x * plane.second, true);
        // scan_2.getSlice<unsigned char>(charSliceImg_2, plane.first, size_2.x * plane.second, true);
        // displayKeypoints(charSliceImg_1, keypoints_1, start_1, end_1);
        // displayKeypoints(charSliceImg_2, keypoints_2, start_2, end_2);

        calculateDescriptors(gaussians_1, DoG_1, keypoints_1, descriptors_1);
        calculateDescriptors(gaussians_2, DoG_2, keypoints_2, descriptors_2);
        
        matcher.match(descriptors_1, descriptors_2, matches);
        
        printf("Total %lu matches on plane %i:%.2f\n", matches.size(), plane.first, plane.second);

        // displayMatches(charSliceImg_1, charSliceImg_2, keypoints_1, keypoints_2, matches, maxOverlap);

        for (const cv::DMatch match : matches) {
            auto kp_1 = keypoints_1[match.queryIdx].pt;
            auto kp_2 = keypoints_2[match.trainIdx].pt;
            offsetsZ.push_back(kp_2.y - kp_1.y + maxOverlap);

            // if (plane.first == 0) {
            //     offsetsY.push_back(kp_2.x - kp_1.x);
            // }
            // else if (plane.first == 1) {
            //     offsetsX.push_back(kp_2.x - kp_1.x);
            // }
        }
    }
    
    // Get optimal offset
    int offsetZ = getMedian(offsetsZ);

    std::vector<std::pair<int, float>> h_planes = {{2, 0.3}, {2, 0.4}, {2, 0.5}, {2, 0.6}, {2, 0.7}};

    for (auto plane : h_planes) {
        DoG_1.clear();
        DoG_2.clear();
        keypoints_1.clear();
        keypoints_2.clear();
        descriptors_1.release();
        descriptors_2.release();
        matches.clear();

        gaussians_1.resize(octavesNum);
        gaussians_2.resize(octavesNum);
        DoG_1.resize(octavesNum);
        DoG_2.resize(octavesNum);

        for (int octave = 0; octave < octavesNum; ++octave) {
            gaussians_1[octave].resize(blurLevelsNum);
            gaussians_2[octave].resize(blurLevelsNum);
            DoG_1[octave].resize(blurLevelsNum - 1);
            DoG_2[octave].resize(blurLevelsNum - 1);
        }

        for (int octave = 0; octave < octavesNum; ++octave) {
            for (int scale_level = 0; scale_level < blurLevelsNum; ++scale_level) {
                VoxelContainer::Vector3 gSize_1 = scanGaussians_1[octave][scale_level].getSize();
                VoxelContainer::Vector3 gSize_2 = scanGaussians_2[octave][scale_level].getSize();
                
                int gOffsetZ = static_cast<float>(gSize_1.z) / static_cast<float>(maxOverlap) * static_cast<float>(offsetZ);
                int slice_id_2 = gOffsetZ * plane.second;
                int slice_id_1 = gOffsetZ - slice_id_2;

                scanGaussians_1[octave][scale_level].getSlice<float>(sliceImg, plane.first, slice_id_1, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(gaussians_1[octave][scale_level]);

                scanGaussians_2[octave][scale_level].getSlice<float>(sliceImg, plane.first, slice_id_2, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(gaussians_2[octave][scale_level]);
            }

            for (int scale_level = 0; scale_level < blurLevelsNum - 1; ++scale_level) {
                VoxelContainer::Vector3 dSize_1 = scanDoGs_1[octave][scale_level].getSize();
                VoxelContainer::Vector3 dSize_2 = scanDoGs_2[octave][scale_level].getSize();

                int dOffsetZ = static_cast<float>(dSize_1.z) / static_cast<float>(maxOverlap) * static_cast<float>(offsetZ);
                int slice_id_2 = dOffsetZ * plane.second;
                int slice_id_1 = dOffsetZ - slice_id_2;

                scanDoGs_1[octave][scale_level].getSlice<float>(sliceImg, plane.first, slice_id_1, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(DoG_1[octave][scale_level]);

                scanDoGs_2[octave][scale_level].getSlice<float>(sliceImg, plane.first, slice_id_2, false);
                cv::Mat_<float>(sliceImg.getHeight(), sliceImg.getWidth(), sliceImg.getData()).copyTo(DoG_2[octave][scale_level]);
            }
        }

        detect(DoG_1, keypoints_1);
        detect(DoG_2, keypoints_2);

        printf("Finded %lu and %lu candidates to keypoints\n", keypoints_1.size(), keypoints_2.size());

        localize(DoG_1, keypoints_1);
        localize(DoG_2, keypoints_2);

        printf("Localized %lu and %lu keypoints\n", keypoints_1.size(), keypoints_2.size());

        orient(gaussians_1, DoG_1, keypoints_1);
        orient(gaussians_2, DoG_2, keypoints_2);

        printf("Oriented %lu and %lu keypoints\n", keypoints_1.size(), keypoints_2.size());

        calculateDescriptors(gaussians_1, DoG_1, keypoints_1, descriptors_1);
        calculateDescriptors(gaussians_2, DoG_2, keypoints_2, descriptors_2);

        // TiffImage<unsigned char> charSliceImg_1, charSliceImg_2;
        // int slice_id_2 = offsetZ * plane.second;
        // int slice_id_1 = size_1.z - offsetZ + slice_id_2;
        // scan_1.getSlice<unsigned char>(charSliceImg_1, plane.first, slice_id_1, true);
        // scan_2.getSlice<unsigned char>(charSliceImg_2, plane.first, slice_id_2, true);
        // displayKeypoints(charSliceImg_1, keypoints_1, 0, size_1.x);
        // displayKeypoints(charSliceImg_2, keypoints_2, 0, size_2.x);
        
        matcher.match(descriptors_1, descriptors_2, matches);

        // displayMatches(charSliceImg_1, charSliceImg_2, keypoints_1, keypoints_2, matches, charSliceImg_1.getHeight());

        printf("Total %lu matches on plane %i:%.2f\n", matches.size(), plane.first, plane.second);

        for (const cv::DMatch match : matches) {
            auto kp_1 = keypoints_1[match.queryIdx].pt;
            auto kp_2 = keypoints_2[match.trainIdx].pt;
            offsetsX.push_back(kp_2.x - kp_1.x);
            offsetsY.push_back(kp_2.y - kp_1.y);
        }
    }

    // Get optimal offsets
    int offsetX = getMedian(offsetsX);
    int offsetY = getMedian(offsetsY);

    scan_2.setEstStitchParams({offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ});

    auto refParams_1 = scan_1.getRefStitchParams();
    auto refParams_2 = scan_2.getRefStitchParams();
    printf("Offsets are %i %i %i. Should be %i %i %i\n", offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ, refParams_2.offsetX - refParams_1.offsetX, refParams_2.offsetY - refParams_1.offsetY, refParams_2.offsetZ - refParams_1.offsetZ);
}


void SIFT3DStitcher::displayKeypoints(TiffImage<unsigned char>& sliceImg, const std::vector<cv::KeyPoint>& keypoints, const int start, const int end) {
    cv::Mat_<unsigned char> slice(end - start, sliceImg.getWidth(), sliceImg.getData() + start * sliceImg.getWidth());
    cv::Mat rgbSlice;
    cv::cvtColor(slice, rgbSlice, cv::COLOR_GRAY2RGB);
    cv::drawKeypoints(rgbSlice, keypoints, rgbSlice, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::resize(rgbSlice, rgbSlice, {0, 0}, 4, 4);
    // cv::namedWindow("Display Keypoints", cv::WINDOW_AUTOSIZE);
    // cv::imshow("Display Keypoints", rgbSlice);
    static int unique_image_id = 7826374;
    cv::imwrite("oriented_keypoints_" + std::to_string(unique_image_id++) + ".png", rgbSlice);
    // cv::waitKey(0);
    // int key = -1;
    // while (key != 'q') key = cv::waitKeyEx(100);
    // cv::destroyAllWindows();
}


void SIFT3DStitcher::displayMatches(TiffImage<unsigned char>& sliceImg_1, TiffImage<unsigned char>& sliceImg_2, const std::vector<cv::KeyPoint>& keypoints_1, const std::vector<cv::KeyPoint>& keypoints_2, const std::vector<cv::DMatch>& matches, const int maxOverlap) {
    cv::Mat_<unsigned char> slice_1(maxOverlap, sliceImg_1.getWidth(), sliceImg_1.getData() + (sliceImg_1.getHeight() - maxOverlap) * sliceImg_1.getWidth());
    cv::Mat_<unsigned char> slice_2(maxOverlap, sliceImg_2.getWidth(), sliceImg_2.getData());
    cv::Mat graySlice_1, graySlice_2, rgbSlice_1, rgbSlice_2;
    cv::normalize(slice_1, graySlice_1, 0, 255, cv::NORM_MINMAX);
    cv::normalize(slice_2, graySlice_2, 0, 255, cv::NORM_MINMAX);
    graySlice_1.convertTo(graySlice_1, CV_8U);
    graySlice_2.convertTo(graySlice_2, CV_8U);
    cv::cvtColor(graySlice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);
    cv::cvtColor(graySlice_2, rgbSlice_2, cv::COLOR_GRAY2RGB);
    // cv::drawKeypoints(rgbSlice, keypoints, rgbSlice, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
    cv::Mat matchResult;
    cv::drawMatches(rgbSlice_1, keypoints_1, rgbSlice_2, keypoints_2, matches, matchResult, cv::Scalar::all(-1), cv::Scalar::all(-1), std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
    cv::namedWindow("Display Matches", cv::WINDOW_AUTOSIZE);
    cv::imshow("Display Matches", matchResult);
    int key = -1;
    while (key != 'q') key = cv::waitKeyEx(100);
    static int unique_image_id = 0;
    cv::imwrite("matches_" + std::to_string(unique_image_id++) + ".png", matchResult);
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


void SIFT3DStitcher::gaussianBlur(const VoxelContainer& src, VoxelContainer& dst, const double sigma, const int start, const int end) {
    int radius = 2 * sigma;
    size_t gSize = 2 * radius + 1;

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
    if (end > 0) {
        size.z = end - start;
    }

    dst.create(size, src.getRange());

    // displaySlice(gaussian);

    for (int sz = 0; sz < size.z; ++sz) {
        std::cout << sz << " of " << size.z << std::endl;
        for (int sy = 0; sy < size.y; ++sy) {
            for (int sx = 0; sx < size.x; ++sx) {
                for (int z = -radius; z <= radius; ++z) {
                    int lz = sz + z + start;
                    if (lz < start || lz >= size.z + start) {
                        continue;
                    }
                    for (int y = -radius; y <= radius; ++y) {
                        // printf()
                        int ly = sy + y;
                        if (ly < 0 || ly >= size.y) {
                            continue;
                        }
                        for (int x = -radius; x <= radius; ++x) {
                            int lx = sx + x;
                            if (lx < 0 || lx >= size.x) {
                                continue;
                            }

                            dst.at(sx, sy, sz) += src.at(lx, ly, lz) * gaussian.at(x + radius, y + radius, z + radius) / (sum * radius * radius);
                        }
                    }
                }
            }
        }
    }

    // displaySlice(src);
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


void SIFT3DStitcher::buildDoG(const VoxelContainer& vol, std::vector<std::vector<VoxelContainer>>& gaussians, std::vector<std::vector<VoxelContainer>>& DoG, const int start, const int end) {
    const double k = std::pow(2, 1 / static_cast<double>(scaleLevelsNum));
    
    gaussians.resize(octavesNum);
    DoG.resize(octavesNum);

    for (int i = 0; i < octavesNum; ++i) {
        gaussians[i].resize(blurLevelsNum);
        DoG[i].resize(blurLevelsNum - 1);
    }

    gaussianBlur(vol, gaussians[0][0], sigma, start, end);

    for (int octave = 0; octave < octavesNum; ++octave) {
        for (int scale_level = 1; scale_level < blurLevelsNum; ++scale_level) {
            std::cout << "DoG: octave " << octave << " of " << octavesNum - 1 << ", scale " << scale_level - 1 << " of " << blurLevelsNum - 2 << std::endl;
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

            if (kp_shift.at<float>(0) < min_shift &&
                kp_shift.at<float>(1) < min_shift &&
                kp_shift.at<float>(2) < min_shift) {
                // Rejecting unstable extrema with low contrast
                float contrast = std::abs(last_kp_val + 0.5 * (grad.at<float>(0) * kp_shift.at<float>(0) +
                                                               grad.at<float>(1) * kp_shift.at<float>(1) +
                                                               grad.at<float>(2) * kp_shift.at<float>(2)));

                if (contrast * scaleLevelsNum < min_contrast) {
                    // printf("Low contrast keypoint discarded %f [%f %f %f] (%f %f %f)\n", contrast, grad.at<float>(0), grad.at<float>(1), grad.at<float>(2), kp_shift.at<float>(0), kp_shift.at<float>(1), kp_shift.at<float>(2));
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
    if (a != 0) {
        return b / a;
    }
    
    return 0;
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

        while (kp.angle < 0) {
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

                while (angle < 0) {
                    angle += 360;
                }

                // Create new keypoint on the same place with other orientation
                keypoints.emplace_back(kp.pt, kp.size, angle, kp.response, kp.octave, kp.class_id);
            }
        }
    }
}


void SIFT3DStitcher::calculateDescriptors(const std::vector<std::vector<cv::Mat>>& gaussians, const std::vector<std::vector<cv::Mat>>& DoG, std::vector<cv::KeyPoint>& keypoints, cv::Mat& descriptors) {
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
                orientation = std::fmod(orientation + 180 + kp.angle, 360);

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
            // printf("%f ", descriptor.at<float>(i));
        //     float desc_val = descriptor.at<float>(i);
        //     if (std::isnan(desc_val)) {
        //         printf("%f ", desc_val);
        //     }
        // }
        // printf("]\n\n[");
    }
}
