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

        size_t volume();
    };
    
    VoxelContainer() = default;
    VoxelContainer(const std::vector<std::string>& fileNames);
    VoxelContainer(float* _data, const Vector3& _size);
    ~VoxelContainer();

    bool loadFromImages(const std::vector<std::string>& fileNames);
    bool loadFromJson(const std::string& fileName);
    void clear();

    bool isEmpty();

    const Vector3& getSize() const;
    const float* getData() const;

    template<typename T>
    void getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange = true);

//    QPixmap getXSlice(const int sliceId); // Sagittal plane
//    QPixmap getYSlice(const int sliceId); // Coronal plane
//    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    void fitToFloatRange(int64_t min, int64_t max);

    template<typename Tin>
    bool readImagesToFloat(const std::vector<std::string> &fileNames);

    Vector3 size = {0, 0, 0};
    float* data = nullptr;
    float rangeMin = 0;
    float rangeMax = 255;
};


template<typename T>
void VoxelContainer::getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange) {
    if (data == nullptr) {
        img.clear();
        return;
    }

    // TEMP
    float newMin = rangeMin;
    float newMax = rangeMax;

    if (fitToRange) {
        newMax = std::numeric_limits<T>::max();
        newMin = std::numeric_limits<T>::min();
    }

    #define FIT_TO_RANGE(X, MIN, MAX) (X - rangeMin) / (rangeMax - rangeMin) * (MAX - MIN) + MIN;

    switch (planeId) {
        case 0: {
            img.resize(size.y, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int y = 0; y < size.y; ++y) {
                    bits[z * size.y + y] = FIT_TO_RANGE(data[z * size.x * size.y + y * size.x + sliceId], newMin, newMax);
                }
            }

            return;
        }

        case 1: {
            img.resize(size.x, size.z);
            T* bits = img.getData();

            for (int z = 0; z < size.z; ++z) {
                for (int x = 0; x < size.x; ++x) {
                    bits[z * size.x + x] = FIT_TO_RANGE(data[z * size.x * size.y + sliceId * size.y + x], newMin, newMax);
                }
            }

            return;
        }

        case 2: {
            img.resize(size.x, size.y);
            T* bits = img.getData();

            for (int y = 0; y < size.y; ++y) {
                for (int x = 0; x < size.x; ++x) {
                    bits[y * size.x + x] = FIT_TO_RANGE(data[sliceId * size.x * size.y + y * size.x + x], newMin, newMax);
                }
            }

            return;
        }

        default:
            img.clear();
            return;
    }

    #undef FIT_TO_RANGE
}


#endif // VOXELCONTAINER_H
