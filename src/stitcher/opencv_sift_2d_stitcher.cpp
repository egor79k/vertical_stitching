#include "opencv_sift_2d_stitcher.h"
#include <algorithm>
// #include <>


void filterMatches(const std::vector<cv::DMatch>& matches, std::vector<cv::DMatch>& goodMatches) {
    double min_dist = std::numeric_limits<double>::max();;
    
    for (int i = 0; i < matches.size(); i++)
    {
        double dist = matches[i].distance;

        if (dist < min_dist) {
            min_dist = dist;
        }
    }

    for (int i = 0; i < matches.size(); i++)
    {
        // if (matches[i].distance < 3 * min_dist)
        // {
        goodMatches.push_back(matches[i]);
        // }
    }

    // goodMatches = matches;
}


float getMedian(std::vector<float>& array) {
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

    float min = 10000;
    int min_loc = size / 2;

    for (int i = 0; i < size; ++i) {
        float val = std::abs(array[i]);
        if (val < min) {
            min = val;
            min_loc = i;
        }
    }

    int mid = (min_loc + size / 2) / 2;

    return (array[mid - 1] + array[mid] + array[mid + 1]) / 3;
}


void OpenCVSIFT2DStitcher::estimateStitchParams(const VoxelContainer& scan_1, VoxelContainer& scan_2) {
    int totalMatches = 0;
    float distancesSum = 0;
    std::vector<float> offsetsX;
    std::vector<float> offsetsY;
    
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
    cv::Mat descriptor_1, descriptor_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches, goodMatches;
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    std::vector<int> planes = {0, 1, 3, 4};

    for (int plane : planes) {
        int slice_id = size_1.x / 2;
        scan_1.getSlice<uint8_t>(sliceImg_1, plane, slice_id, true);
        cv::Mat_<unsigned char> slice_1(maxOverlap, size_1.y, sliceImg_1.getData() + (size_1.z - maxOverlap) * size_1.y);
        // cv::Mat_<unsigned char> slice_1(size_1.x, size_1.y, sliceImg_1.getData());
        sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptor_1);

        cv::Mat rgbSlice_1, rgbSlice_2, match_result;
        cv::cvtColor(slice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);

        scan_2.getSlice<uint8_t>(sliceImg_2, plane, slice_id, true);
        cv::Mat_<unsigned char> slice_2(maxOverlap, size_2.y, sliceImg_2.getData());
        // cv::Mat_<unsigned char> slice_2(size_2.x, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptor_2);

        matcher.match(descriptor_1, descriptor_2, matches);
        filterMatches(matches, goodMatches);

        size_t goodMatchesNum = goodMatches.size();

        totalMatches += goodMatchesNum;
        for (int i = 0; i < goodMatchesNum; ++i) {
            auto kp_1 = keypoints_1[goodMatches[i].queryIdx].pt;
            auto kp_2 = keypoints_2[goodMatches[i].trainIdx].pt;
            float distance = kp_2.y - kp_1.y + maxOverlap;
            printf("distance: %f\n", distance);
            distancesSum += distance;

            if (plane == 0) {
                offsetsY.push_back(kp_2.x - kp_1.x);
            }
            else if (plane == 1) {
                offsetsX.push_back(kp_2.x - kp_1.x);
            }

            // int mId_1 = goodMatches[i].queryIdx;
            // int mId_2 = goodMatches[i].trainIdx;

            // putchar('\n');
            // for (int j = 0; j < descriptor_1.cols; ++j) {
            //     printf("%f ", descriptor_1.at<float>(mId_1, j));
            // }
            // putchar('\n');
            // for (int j = 0; j < descriptor_2.cols; ++j) {
            //     printf("%f ", descriptor_2.at<float>(mId_2, j));
            // }
            // putchar('\n');
        }

        printf("%s %lu %s %lu\n", "Total matches:", matches.size(), "good matches:", goodMatchesNum);

        cv::cvtColor(slice_2, rgbSlice_2, cv::COLOR_GRAY2RGB);
        
        cv::namedWindow("Display Matches", cv::WINDOW_AUTOSIZE);
        cv::drawMatches(rgbSlice_1, keypoints_1, rgbSlice_2, keypoints_2, goodMatches, match_result);
        cv::imshow("Display Matches", match_result);
        int key = -1;
        while (key != 'q') key = cv::waitKeyEx(100);

        keypoints_2.clear();
        matches.clear();
        goodMatches.clear();
    }

    cv::destroyAllWindows();

    int optimalOverlap = 0;

    if (totalMatches > 0) {
        optimalOverlap = distancesSum / totalMatches;
    }

    int offsetX = getMedian(offsetsX);
    int offsetY = getMedian(offsetsY);

    printf("Offsets are %i %i. Should be %i %i\n", offsetX, offsetY, scan_2.getRefStitchParams().offsetX - scan_1.getRefStitchParams().offsetX, scan_2.getRefStitchParams().offsetY - scan_1.getRefStitchParams().offsetY);

    printf("%s %i %s %f/%i\n", "Optimal overlap:", optimalOverlap, "DST:", distancesSum, totalMatches);

    scan_2.setEstStitchParams({offsetX, offsetY, static_cast<int>(size_1.z) - optimalOverlap});
}


/*
int OpenCVSIFT2DStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    float maxMatchesNum = 0;
    int optimalOverlap = 0;

    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    TiffImage<uint8_t> sliceImg_1;
    TiffImage<uint8_t> sliceImg_2;

    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    cv::Mat descriptor_1, descriptor_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches, goodMatches;
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();

    scan_1.getSlice<uint8_t>(sliceImg_1, 2, size_1.z - 1, true);
    cv::Mat_<unsigned char> slice_1(size_1.x, size_1.y, sliceImg_1.getData());
    sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptor_1);

    cv::Mat rgbSlice_1, rgbSlice_2, match_result;
    cv::cvtColor(slice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);

    for (int overlap = minOverlap; overlap < maxOverlap; overlap += offsetStep) {
        scan_2.getSlice<uint8_t>(sliceImg_2, 2, overlap - 1, true);
        cv::Mat_<unsigned char> slice_2(size_2.x, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptor_2);

        matcher.match(descriptor_1, descriptor_2, matches);
        filterMatches(matches, goodMatches);

        size_t goodMatchesNum = goodMatches.size();

        if (goodMatchesNum > maxMatchesNum) {
            maxMatchesNum = goodMatchesNum;
            optimalOverlap = overlap;
        }

        printf("%s %lu %s %lu\n", "Total matches:", matches.size(), "good matches:", goodMatchesNum);

        cv::cvtColor(slice_2, rgbSlice_2, cv::COLOR_GRAY2RGB);
        
        cv::namedWindow("Display Matches", cv::WINDOW_AUTOSIZE);
        cv::drawMatches(rgbSlice_1, keypoints_1, rgbSlice_2, keypoints_2, goodMatches, match_result);
        cv::imshow("Display Matches", match_result);
        int key = -1;
        while (key != 'q') key = cv::waitKeyEx(100);

        keypoints_2.clear();
        matches.clear();
        goodMatches.clear();
    }

    cv::destroyAllWindows();

    printf("%s %i\n", "Optimal overlap:", optimalOverlap);

    return optimalOverlap;
}*/
