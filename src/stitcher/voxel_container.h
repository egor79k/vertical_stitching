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

    struct Range {
        float min;
        float max;

        float fit(float val, const Range nr);
    };
    
    VoxelContainer() = default;
    VoxelContainer(const std::vector<std::string>& fileNames);
    VoxelContainer(float* _data, const Vector3& _size, const Range& _range);
    ~VoxelContainer();

    bool loadFromImages(const std::vector<std::string>& fileNames);
    bool loadFromJson(const std::string& fileName);
    void clear();

    bool isEmpty();

    const float* getData() const;
    const Vector3& getSize() const;
    const Range& getRange() const;

    template<typename T>
    void getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange = true);

//    QPixmap getXSlice(const int sliceId); // Sagittal plane
//    QPixmap getYSlice(const int sliceId); // Coronal plane
//    QPixmap getZSlice(const int sliceId); // Transverse plane

private:
    bool readImages(const std::vector<std::string> &fileNames);

    float* data = nullptr;
    Vector3 size = {0, 0, 0};
    Range range = {0, 0};
};


template<typename T>
void VoxelContainer::getSlice(TiffImage<T>& img, const int planeId, const int sliceId, bool fitToRange) {
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

        default:
            img.clear();
            return;
    }
}


#endif // VOXELCONTAINER_H
