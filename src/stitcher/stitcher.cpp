#include <iostream>
#include <cstring>
#include <limits>
#include <opencv2/opencv.hpp>
#include "stitcher.h"


std::shared_ptr<VoxelContainer> StitcherImpl::stitch(const std::vector<std::shared_ptr<VoxelContainer>>& partialScans) {
    std::shared_ptr<VoxelContainer> result = partialScans[0];

    for (int scan_id = 1; scan_id < partialScans.size(); ++scan_id) {
        result = stitch(*result, *partialScans[scan_id]);
        if (result == nullptr) {
            return nullptr;
        }
    }

    return result;
}


VoxelContainer::Range StitcherImpl::getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Range r1 = scan_1.getRange();
    VoxelContainer::Range r2 = scan_2.getRange();

    return {std::min(r1.min, r2.min), std::max(r1.max, r2.max)};
}


std::shared_ptr<VoxelContainer> SimpleStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();
    VoxelContainer::Vector3 gap = {size_1.x, size_1.y, 5};

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        // QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z + gap.z};
    VoxelContainer::Range stitchedRange = getStitchedRange(scan_1, scan_2);
    float* stitchedData = new float[stitchedSize.volume()];

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));

    // Fill the gap with black
    float* gapData = stitchedData + size_1.volume();
    for (int i = 0; i < gap.volume(); ++ i) {
        gapData[i] = stitchedRange.min;
    }

    memcpy(stitchedData + size_1.volume() + gap.volume(), scan_2.getData(), size_2.volume() * sizeof(float));

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, stitchedRange);
}


const int OverlapDifferenceStitcher::minOverlap = 5;
const int OverlapDifferenceStitcher::maxOverlap = 30;
const int OverlapDifferenceStitcher::offsetStep = 1;


std::shared_ptr<VoxelContainer> OverlapDifferenceStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
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


float OverlapDifferenceStitcher::countDifference(const VoxelContainer& scan_1, const VoxelContainer& scan_2, const int overlap) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();

    const float* data_1 = scan_1.getData();
    const float* data_2 = scan_2.getData();

    float diff = 0;

    int scanOffset = size_1.x * size_1.y * (size_1.z - overlap);
    int offsetVolume = overlap * size_1.x * size_1.y;

    for (int i = 0; i < offsetVolume; ++i) {
        diff += (data_1[i + scanOffset] - data_2[i]) * (data_1[i + scanOffset] - data_2[i]);
    }

    return diff / overlap;
}


int OverlapDifferenceStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    float minDiff = std::numeric_limits<float>::max();
    int optimalOverlap = 0;

    for (int overlap = minOverlap; overlap < maxOverlap; overlap += offsetStep) {
        float currDiff = countDifference(scan_1, scan_2, overlap);
        
        if (currDiff < minDiff) {
            minDiff = currDiff;
            optimalOverlap = overlap;
        }

        printf("%s %i %s %f\n", "Overlap:", overlap, "diff:", currDiff);
    }

    printf("%s %i\n", "Optimal overlap:", optimalOverlap);

    return optimalOverlap;
}


const int CVSIFT2DStitcher::minOverlap = 5;
const int CVSIFT2DStitcher::maxOverlap = 30;
const int CVSIFT2DStitcher::offsetStep = 1;


std::shared_ptr<VoxelContainer> CVSIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
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
        if (matches[i].distance < 3 * min_dist)
        {
            goodMatches.push_back(matches[i]);
        }
    }

    // goodMatches = matches;
}


int CVSIFT2DStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    int totalMatches = 0;
    float distancesSum = 0;

    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

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
        // cv::Mat_<unsigned char> slice_1(maxOverlap, size_1.y, sliceImg_1.getData() + (size_1.z - maxOverlap) * size_1.y);
        cv::Mat_<unsigned char> slice_1(size_1.x, size_1.y, sliceImg_1.getData());
        sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptor_1);

        cv::Mat rgbSlice_1, rgbSlice_2, match_result;
        cv::cvtColor(slice_1, rgbSlice_1, cv::COLOR_GRAY2RGB);

        scan_2.getSlice<uint8_t>(sliceImg_2, plane, slice_id, true);
        // cv::Mat_<unsigned char> slice_2(maxOverlap, size_2.y, sliceImg_2.getData());
        cv::Mat_<unsigned char> slice_2(size_2.x, size_2.y, sliceImg_2.getData());
        sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptor_2);

        matcher.match(descriptor_1, descriptor_2, matches);
        filterMatches(matches, goodMatches);

        size_t goodMatchesNum = goodMatches.size();

        totalMatches += goodMatchesNum;
        for (int i = 0; i < goodMatchesNum; ++i) {
            auto kp_1 = keypoints_1[goodMatches[i].queryIdx].pt;
            auto kp_2 = keypoints_2[goodMatches[i].trainIdx].pt;
            float distance = (kp_2.y - kp_1.y + maxOverlap) / 2;
            printf("distance: %f\n", distance);
            distancesSum += distance;
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

    int optimalOverlap = distancesSum / totalMatches;

    printf("%s %i %s %f/%i\n", "Optimal overlap:", optimalOverlap, "DST:", distancesSum, totalMatches);

    return optimalOverlap;
}


/*
int SIFT2DStitcher::determineOptimalOverlap(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
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
