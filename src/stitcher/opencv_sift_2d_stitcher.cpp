#include "opencv_sift_2d_stitcher.h"
#include <algorithm>


float OpenCVSIFT2DStitcher::getMedian(std::vector<float>& array) {
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

    return array[size / 2];
}


void OpenCVSIFT2DStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    std::vector<float> offsetsX;
    std::vector<float> offsetsY;
    std::vector<float> offsetsZ;
    
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    const int refOffsetZ = scan_2.getRefStitchParams().offsetZ;
    int maxOverlap = size_2.z / 2;

    if (refOffsetZ > 0) {
        maxOverlap = size_1.z - refOffsetZ;
        int maxDeviation = std::max(5, maxOverlap / 5);
        maxOverlap += maxDeviation;
    }

    TiffImage<uint8_t> sliceImg_1;
    TiffImage<uint8_t> sliceImg_2;

    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    cv::Mat descriptors_1, descriptors_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches;
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    std::vector<std::pair<int, float>> planes = {{0, 0.4}, {0, 0.5}, {0, 0.6}, {1, 0.4}, {1, 0.5}, {1, 0.6}, {3, 0}, {4, 0}};

    for (auto plane : planes) {
        // Find keypoints and compute descriptors on the middle current plane
        int slice_id = size_1.x * plane.second;
        scan_1.getSlice<uint8_t>(sliceImg_1, plane.first, slice_id, true);
        cv::Mat_<unsigned char> slice_1(maxOverlap, size_1.y, sliceImg_1.getData() + (size_1.z - maxOverlap) * size_1.y);
        sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptors_1);

        scan_2.getSlice<uint8_t>(sliceImg_2, plane.first, slice_id, true);
        cv::Mat_<unsigned char> slice_2(maxOverlap, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptors_2);

        matcher.match(descriptors_1, descriptors_2, matches);

        // Put distances between matched points to offsets vectors
        for (const cv::DMatch match : matches) {
            auto kp_1 = keypoints_1[match.queryIdx].pt;
            auto kp_2 = keypoints_2[match.trainIdx].pt;
            offsetsZ.push_back(kp_2.y - kp_1.y + maxOverlap);
        }

        printf("Total %lu matches on plane %i:%.2f\n", matches.size(), plane.first, plane.second);

        // Display matches
        // cv::Mat rgbSlice_1, rgbSlice_2, match_result;
        // cv::cvtColor(slice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);
        // cv::cvtColor(slice_2, rgbSlice_2, cv::COLOR_GRAY2RGB);
        // cv::drawMatches(rgbSlice_1, keypoints_1, rgbSlice_2, keypoints_2, matches, match_result);
        // cv::imshow("Display Matches", match_result);
        // int key = -1;
        // while (key != 'q') key = cv::waitKeyEx(100);

        // Clear current plane features
        keypoints_1.clear();
        keypoints_2.clear();
        matches.clear();
        descriptors_1.release();
        descriptors_2.release();
    }

    // Get optimal offset
    int offsetZ = getMedian(offsetsZ);

    std::vector<std::pair<int, float>> h_planes = {{2, 0.3}, {2, 0.4}, {2, 0.5}, {2, 0.6}, {2, 0.7}};

    for (auto plane : h_planes) {
        // Find keypoints and compute descriptors on the middle current plane
        int slice_id_2 = offsetZ * plane.second;
        int slice_id_1 = size_1.z - offsetZ + slice_id_2;
        scan_1.getSlice<uint8_t>(sliceImg_1, plane.first, slice_id_1, true);
        cv::Mat_<unsigned char> slice_1(size_1.x, size_1.y, sliceImg_1.getData());
        sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptors_1);

        scan_2.getSlice<uint8_t>(sliceImg_2, plane.first, slice_id_2, true);
        cv::Mat_<unsigned char> slice_2(size_2.x, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptors_2);

        matcher.match(descriptors_1, descriptors_2, matches);

        // Put distances between matched points to offsets vectors
        for (const cv::DMatch match : matches) {
            auto kp_1 = keypoints_1[match.queryIdx].pt;
            auto kp_2 = keypoints_2[match.trainIdx].pt;
            offsetsX.push_back(kp_2.x - kp_1.x);
            offsetsY.push_back(kp_2.y - kp_1.y);
        }

        printf("Total %lu matches on plane %i:%.2f\n", matches.size(), plane.first, plane.second);

        // Clear current plane features
        keypoints_1.clear();
        keypoints_2.clear();
        matches.clear();
        descriptors_1.release();
        descriptors_2.release();
    }

    // Get optimal offset
    int offsetX = getMedian(offsetsX);
    int offsetY = getMedian(offsetsY);

    scan_2.setEstStitchParams({offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ});

    auto refParams_1 = scan_1.getRefStitchParams();
    auto refParams_2 = scan_2.getRefStitchParams();
    printf("Offsets are %i %i %i. Should be %i %i %i\n", offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ, refParams_2.offsetX - refParams_1.offsetX, refParams_2.offsetY - refParams_1.offsetY, refParams_2.offsetZ - refParams_1.offsetZ);
}
