#ifndef VOXELCONTAINER_H
#define VOXELCONTAINER_H

#include <string>
#include <vector>
#include "tiff_image.h"


class VoxelContainer {
public:
    struct Vector3 {
        size_t x;
        size_t y;
        size_t z;

        size_t volume() const;
    };

    struct Range {
        float min;
        float max;

        float fit(float val, const Range nr) const;
    };

    struct StitchParams {
        int offsetX = 0;
        int offsetY = 0;
        int offsetZ = 0;
    };
    
    VoxelContainer() = default;
    VoxelContainer(const std::vector<std::string>& fileNames);
    VoxelContainer(float* _data, const Vector3& _size, const Range& _range);
    VoxelContainer(const Vector3& _size, const Range& _range = {0, 0});
    ~VoxelContainer();

    bool loadFromImages(const std::vector<std::string>& fileNames);
    bool loadFromJson(const std::string& fileName);
    bool saveToJson(const std::string& dirName);
    void create(const Vector3& _size, const Range& _range = {0, 0});
    void clear();

    bool isEmpty();

    float& at(const int x, const int y, const int z);
    const float& at(const int x, const int y, const int z) const;
    float* getData() const;
    const Vector3& getSize() const;
    const Range& getRange() const;
    const StitchParams& getStitchParams() const;
    void setStitchParams(const StitchParams& params);

    template<typename T>
    void getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange = true) const;

//    QPixmap getXSlice(const int sliceId); // Sagittal plane
//    QPixmap getYSlice(const int sliceId); // Coronal plane
//    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    bool readImages(const std::vector<std::string>& fileNames);
    bool writeImages(const std::string& dirName);

    float* data = nullptr;
    Vector3 size = {0, 0, 0};
    Range range = {0, 0};
    StitchParams trueParams;
    StitchParams estimatedParams;
};


void substract(const VoxelContainer& a, const VoxelContainer& b, VoxelContainer& dst);


template<typename T>
void VoxelContainer::getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange) const {
    if (data == nullptr) {
        img.clear();
        return;
    }

    Range newRange = range;

    if (fitToRange) {
        newRange.max = std::numeric_limits<T>::max();
        newRange.min = std::numeric_limits<T>::min();
    }

    switch (planeId) {
        // Sagittal plane
        case 0: {
            img.resize(size.y, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int y = 0; y < size.y; ++y) {
                    bits[z * size.y + y] = range.fit(data[z * size.x * size.y + y * size.x + sliceId], newRange);
                }
            }

            return;
        }

        // Coronal plane
        case 1: {
            img.resize(size.x, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int x = 0; x < size.x; ++x) {
                    bits[z * size.x + x] = range.fit(data[z * size.x * size.y + sliceId * size.y + x], newRange);
                }
            }

            return;
        }

        // Transverse plane
        case 2: {
            img.resize(size.x, size.y);
            T* bits = img.getData();

            for (int y = 0; y < size.y; ++y) {
                for (int x = 0; x < size.x; ++x) {
                    bits[y * size.x + x] = range.fit(data[sliceId * size.x * size.y + y * size.x + x], newRange);
                }
            }

            return;
        }

        // Diagonal plane
        case 3: {
            img.resize(size.y, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int y = 0; y < size.y; ++y) {
                    bits[z * size.y + y] = range.fit(data[z * size.x * size.y + y * size.x + y], newRange);
                }
            }

            return;
        }

        // Diagonal plane
        case 4: {
            img.resize(size.x, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int x = 0; x < size.x; ++x) {
                    bits[z * size.x + x] = range.fit(data[z * size.x * size.y + x * size.y + size.x - x - 1], newRange);
                }
            }

            return;
        }

        default:
            img.clear();
            return;
    }
}


#endif // VOXELCONTAINER_H
