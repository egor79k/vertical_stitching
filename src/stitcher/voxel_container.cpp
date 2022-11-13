#include <algorithm>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>
#include "voxel_container.h"

using json = nlohmann::json;


size_t VoxelContainer::Vector3::volume() const {
    return x * y * z;
}


float VoxelContainer::Range::fit(float val, const Range nr) const {
    return (val - min) / (max - min) * (nr.max - nr.min) + nr.min;
}


VoxelContainer::VoxelContainer(const std::vector<std::string>& fileNames) {
    loadFromImages(fileNames);
}


VoxelContainer::VoxelContainer(float* _data, const Vector3& _size, const Range& _range) :
    data(_data),
    size(_size),
    range(_range) {}


VoxelContainer::~VoxelContainer() {
    clear();
}


bool VoxelContainer::loadFromImages(const std::vector<std::string>& fileNames) {
    clear();
    
    if (!TiffImage<float>::getSizeFromFile(fileNames.front().data(), size.x, size.y)) {
        return false;
    }

    size.z = fileNames.size();

    readImages(fileNames);

    auto rng = std::minmax_element(data, data + size.volume());
    range = {*rng.first, *rng.second};
    printf("Range: %f, %f\n", range.min, range.max);

    return true;
}


bool VoxelContainer::loadFromJson(const std::string& fileName) {
    // Open parameters file
    std::ifstream fs(fileName);
    if(!fs) {
        printf("%s %s\n", "Unable to open file", fileName.data());
        return false;
    }

    // Parse JSON parameters
    json data = json::parse(fs, nullptr, false);
    if (data.is_discarded()) {
        printf("Parse error");
        return false;
    }

    size.x = data["width"].get<int>();
    size.y = data["depth"].get<int>();
    size.z = data["height"].get<int>();
    range.min = data["range_min"].get<float>();
    range.max = data["range_max"].get<float>();
    std::string format = data["format"].get<std::string>();

    std::string imgPath = fileName.substr(0, fileName.find_last_of('/') + 1);
    std::vector<std::string> imgNames;

    // Create image files list
    for (int i = 0; i < size.z; ++i) {
        imgNames.push_back(imgPath + std::to_string(i) + format);
    }

    return readImages(imgNames);
}


void VoxelContainer::clear() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
        size = {0, 0, 0};
        range = {0, 0};
    }
}


bool VoxelContainer::isEmpty() {
    return data == nullptr;
}


const float* VoxelContainer::getData() const {
    return data;
}


const VoxelContainer::Vector3& VoxelContainer::getSize() const {
    return size;
}


const VoxelContainer::Range& VoxelContainer::getRange() const {
    return range;
}


bool VoxelContainer::readImages(const std::vector<std::string> &fileNames) {
    data = new float[size.volume()];

    if (data == nullptr) {
        return false;
    }

    size_t width = 0;
    size_t height = 0;

    for (int i = 0; i < size.z; ++i) {
        if (!TiffImage<float>::readFromFile(fileNames.at(i).data(), data + i * size.x * size.y, width, height)) {
            return false;
        }

        if (width != size.x || height != size.y) {
            return false;
        }
    }

    return true;
}


/*
QPixmap VoxelContainer::getXSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(size.y, size.z, QImage::Format_Grayscale8);
    uchar* bits = img.bits();

    for (int z = 0; z < size.z; ++z) {
        for (int y = 0; y < size.y; ++y) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * size.y + y) * bytesPerPixel + byte] = data[(z * size.x * size.y + sliceId * size.x + y) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getYSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    QImage img(size.x, size.z, QImage::Format_Grayscale8);
    uchar* bits = img.bits();

    for (int z = 0; z < size.z; ++z) {
        for (int x = 0; x < size.x; ++x) {
            for (int byte = 0; byte < bytesPerPixel; ++byte) {
                bits[(z * size.x + x) * bytesPerPixel + byte] = data[(z * size.x * size.y + sliceId * size.y + x) * bytesPerPixel + byte];
            }
        }
    }

    return QPixmap::fromImage(img);
}


QPixmap VoxelContainer::getZSlice(const int sliceId) {
    Q_ASSERT(data != nullptr);
    return QPixmap::fromImage(QImage(data + sliceId * size.x * size.y * bytesPerPixel, size.x, size.y, size.x * bytesPerPixel, QImage::Format_Grayscale8));
}
*/
