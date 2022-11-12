#include <iostream>
#include <cstring>
#include <limits>
#include <vector>
#include <opencv2/opencv.hpp>
#include "stitcher.h"

//QSharedPointer<VoxelContainer> StitcherImpl::stitch(const QList<std::shared_ptr<VoxelContainer>>& partialScans) {}


VoxelContainer::Range StitcherImpl::getStitchedRange(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Range r1 = scan_1.getRange();
    VoxelContainer::Range r2 = scan_2.getRange();

    return {std::min(r1.min, r2.min), std::max(r1.max, r2.max)};
}


std::shared_ptr<VoxelContainer> SimpleStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    if (size_1.x != size_2.x || size_1.y != size_2.y) {
        // QMessageBox::information(0, "Stitching error", "Failed to stitch scans due to different sizes.");
        return nullptr;
    }

    VoxelContainer::Vector3 stitchedSize = {size_1.x, size_1.y, size_1.z + size_2.z};

    float* stitchedData = new float[stitchedSize.volume()];

    memcpy(stitchedData, scan_1.getData(), size_1.volume() * sizeof(float));
    memcpy(stitchedData + size_1.volume(), scan_2.getData(), size_2.volume() * sizeof(float));

    return std::make_shared<VoxelContainer>(stitchedData, stitchedSize, getStitchedRange(scan_1, scan_2));
}


const int OverlapDifferenceStitcher::minOverlap = 1;
const int OverlapDifferenceStitcher::maxOverlap = 20;
const int OverlapDifferenceStitcher::offsetStep = 1;


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

    return diff;
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
    }

    return optimalOverlap;
}


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


std::shared_ptr<VoxelContainer> SIFT2DStitcher::stitch(const VoxelContainer& scan_1, const VoxelContainer& scan_2) {
    VoxelContainer::Vector3 size_1 = scan_1.getSize();
    VoxelContainer::Vector3 size_2 = scan_2.getSize();

    TiffImage<uint8_t> slice_img_1;
    TiffImage<uint8_t> slice_img_2;

    scan_1.getSlice<uint8_t>(slice_img_1, 2, size_1.z - 1, true);
    scan_2.getSlice<uint8_t>(slice_img_2, 2, 0, true);

    cv::Mat_<unsigned char> slice_1(size_1.x, size_1.y, slice_img_1.getData());
    cv::Mat_<unsigned char> slice_2(size_2.x, size_2.y, slice_img_2.getData());
    
    // cv::namedWindow("Display Image", cv::WINDOW_AUTOSIZE );
    // cv::imshow("Display Image", slice_1);
    // int key = -1;
    // while (key != 'q') key = cv::waitKeyEx(100);
    // cv::imshow("Display Image", slice_2);
    // key = -1;
    // while (key != 'q') key = cv::waitKeyEx(100);
    // cv::destroyAllWindows();

    std::vector<cv::KeyPoint> keypoints_1, keypoints_2;
    cv::Mat descriptor_1, descriptor_2;
    cv::BFMatcher matcher;
    std::vector<cv::DMatch> matches;
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();

    sift->detectAndCompute(slice_1, cv::Mat(), keypoints_1, descriptor_1);
    sift->detectAndCompute(slice_2, cv::Mat(), keypoints_2, descriptor_2);

    matcher.match(descriptor_1, descriptor_2, matches);

    std::cout << matches.size() << std::endl;

    return std::make_shared<VoxelContainer>();
}