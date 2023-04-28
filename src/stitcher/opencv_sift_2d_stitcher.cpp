#include "opencv_sift_2d_stitcher.h"
#include <algorithm>


float OpenCVSIFT2DStitcher::getMedian(std::vector<float>& array) {
    int size = array.size();
    
    if (size == 0) {
        return 0;
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
    const int maxRefDeviation = 5;
    int maxOverlap = size_1.z / 2;

    if (refOffsetZ > 0) {
        maxOverlap = size_1.z - refOffsetZ + maxRefDeviation;
    }

    TiffImage<uint8_t> sliceImg_1;
    TiffImage<uint8_t> sliceImg_2;

    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    cv::Mat descriptors_1, descriptors_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches;
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    std::vector<int> planes = {0, 1, 3, 4};

    for (int plane : planes) {
        // Find keypoints and compute descriptors on the middle current plane
        int slice_id = size_1.x / 2;
        scan_1.getSlice<uint8_t>(sliceImg_1, plane, slice_id, true);
        cv::Mat_<unsigned char> slice_1(maxOverlap, size_1.y, sliceImg_1.getData() + (size_1.z - maxOverlap) * size_1.y);
        sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptors_1);

        scan_2.getSlice<uint8_t>(sliceImg_2, plane, slice_id, true);
        cv::Mat_<unsigned char> slice_2(maxOverlap, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptors_2);

        matcher.match(descriptors_1, descriptors_2, matches);

        // Put distances between matched points to offsets vectors
        for (const cv::DMatch match : matches) {
            auto kp_1 = keypoints_1[match.queryIdx].pt;
            auto kp_2 = keypoints_2[match.trainIdx].pt;

            offsetsZ.push_back(kp_2.y - kp_1.y + maxOverlap);

            if (plane == 0) {
                offsetsY.push_back(kp_2.x - kp_1.x);
            }
            else if (plane == 1) {
                offsetsX.push_back(kp_2.x - kp_1.x);
            }
        }

        printf("Total %lu matches on plane %i\n", matches.size(), plane);

        // Display matches
        cv::Mat rgbSlice_1, rgbSlice_2, match_result;
        cv::cvtColor(slice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);
        cv::cvtColor(slice_2, rgbSlice_2, cv::COLOR_GRAY2RGB);
        cv::drawMatches(rgbSlice_1, keypoints_1, rgbSlice_2, keypoints_2, matches, match_result);
        cv::imshow("Display Matches", match_result);
        int key = -1;
        while (key != 'q') key = cv::waitKeyEx(100);

        // Clear current plane features
        keypoints_1.clear();
        keypoints_2.clear();
        matches.clear();
        descriptors_1.release();
        descriptors_2.release();
    }

    cv::destroyAllWindows();

    // Get optimal offsets
    int offsetX = getMedian(offsetsX);
    int offsetY = getMedian(offsetsY);
    int offsetZ = getMedian(offsetsZ);

    scan_2.setEstStitchParams({offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ});

    auto refParams_1 = scan_1.getRefStitchParams();
    auto refParams_2 = scan_2.getRefStitchParams();
    printf("Offsets are %i %i %i. Should be %i %i %i\n", offsetX, offsetY, static_cast<int>(size_1.z) - offsetZ, refParams_2.offsetX - refParams_1.offsetX, refParams_2.offsetY - refParams_1.offsetY, refParams_2.offsetZ - refParams_1.offsetZ);
}
