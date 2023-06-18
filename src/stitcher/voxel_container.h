#ifndef VOXELCONTAINER_H
#define VOXELCONTAINER_H

#include <string>
#include <vector>
#include "tiff_image.h"


/**
 * \brief Special data structure for storing 3D reconstructions.
 * VoxelContainer stores a 3D volume of CT reconstruction as an one-dimensional array of float values representing each voxel. There are several ways to initialize it:
 * - Create an empty one using VoxelContainer().
 * - Read it from the list of images using VoxelContainer(const std::vector<std::string>&) for new or loadFromImages(const std::vector<std::string>&) for an existing one.
 * - Create it on existing data using VoxelContainer(float*, const Vector3&, const Range&, const StitchParams&).
 * - Read it from the special parameters file using loadFromJson(const std::string&).
 * - Reallocate empty memory using create(const Vector3&, const Range&).
 */
class VoxelContainer {
public:
    /**
     * \brief Structure for storing 3D container size.
     */
    struct Vector3 {
        size_t x;
        size_t y;
        size_t z;

        /**
         * \brief Calculates container volume.
         * 
         * \return Container volume.
         */
        size_t volume() const;
    };

    /**
     * \brief Structure for storing range of values in reconstruction data.
     */
    struct Range {
        float min;
        float max;

        /**
         * \brief Converts value from the current range to the new one.
         * 
         * \param[in] val Value to be converted
         * \param[in] nr New range
         * \return Value converted to the new range
         */
        float fit(float val, const Range nr) const;
    };

    /**
     * \brief Structure for storing transformation params of the reconstruction.
     */
    struct StitchParams {
        int offsetX;
        int offsetY;
        int offsetZ;
    };
    
    /**
     * \brief Default constructor. Creates an empty instance.
     */
    VoxelContainer() = default;

    /**
     * \brief Constructs reconstruction from the list of layer's images.
     * 
     * \param[in] fileNames List of image names
     */
    VoxelContainer(const std::vector<std::string>& fileNames);

    /**
     * \brief Constructs container on existing data.
     * 
     * \param[in] _data Allocated memory buffer of _size.volume() size
     * \param[in] _size Size of the given data
     * \param[in] _range Range of the given data
     * \param[in] _refParams Rederence transformation parameters
     */
    VoxelContainer(float* _data, const Vector3& _size, const Range& _range, const StitchParams& _refParams);

    /**
     * \brief Constructs container of given size.
     * 
     * \param[in] _size Size to allocate
     * \param[in] _range Initial range of data
     */
    VoxelContainer(const Vector3& _size, const Range& _range = {0, 0});

    /**
     * \brief Destructor.
     */
    ~VoxelContainer();

    /**
     * \brief Reads reconstruction from horizontal slice images.
     * 
     * \param[in] fileNames List of images paths
     * \return True - if success, false - if failed
     */
    bool loadFromImages(const std::vector<std::string>& fileNames);

    /**
     * \brief Reads reconstruction from the special parameters file.
     * 
     * \param[in] fileName Path to the parameters file
     * \return True - if success, false - if failed.
     */
    bool loadFromJson(const std::string& fileName);

    /**
     * \brief Saves reconstruction into special format.
     * 
     * \param[in] fileName Path to the parameters file
     * \return True - if success, false - if failed.
     */
    bool saveToJson(const std::string& dirName);

    /**
     * \brief Reallocates empty memory of given size.
     * 
     * \param[in] _size Size to allocate
     * \param[in] _range Initial range of data
     */
    void create(const Vector3& _size, const Range& _range = {0, 0});

    /**
     * \brief Clears container deallocating memory.
     */
    void clear();

    /**
     * \brief Checks if container is empty.
     *
     * \return True - if empty, false - if not.
     */
    bool isEmpty();

    /**
     * \brief Voxel access by its 3D index.
     *
     * \return Reference to voxel value.
     */
    float& at(const int x, const int y, const int z);

    /**
     * \brief Constant voxel access by its 3D index.
     *
     * \return Constant reference to voxel value.
     */
    const float& at(const int x, const int y, const int z) const;

    /**
     * \brief Voxels buffer access.
     *
     * \return Pointer to the buffer.
     */
    float* getData() const;

    /**
     * \brief Gives container size.
     *
     * \return Container size.
     */
    const Vector3& getSize() const;

    /**
     * \brief Gives container range.
     *
     * \return Container range.
     */
    const Range& getRange() const;

    /**
     * \brief Gives reference stitch parameters.
     *
     * \return Reference stitch parameters.
     */
    const StitchParams& getRefStitchParams() const;

    /**
     * \brief Sets reference stitch parameters.
     *
     * \param[in] params Reference stitch parameters
     */
    void setRefStitchParams(const StitchParams& params);

    /**
     * \brief Gives estimated stitch parameters.
     *
     * \return Estimated stitch parameters.
     */
    const StitchParams& getEstStitchParams() const;

    /**
     * \brief Sets estimated stitch parameters.
     *
     * \param[in] params Estimated stitch parameters
     */
    void setEstStitchParams(const StitchParams& params);

    /**
     * \brief Gives a specified slice of the reconstruction.
     *
     * \param[in] img Destination image of slice
     * \param[in] planeId Index of the slice plane. Might be:
     *  - 0 - Sagittal
     *  - 1 - Coronal
     *  - 2 - Transverse
     *  - 3 - Diagonal 1
     *  - 4 - Diagonal 2
     * \param[in] sliceId Index of the slice. Must be inside container borders
     * \param[in] fitToRange Is needed to fit slice to the range of its data type
     */
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
    StitchParams referenceParams = {0, 0, 0};
    StitchParams estimatedParams = {0, 0, 0};
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
            if (sliceId < 0 || sliceId >= size.x) {
                printf("Error: Slice %i out of X range [0, %lu]\n", sliceId, size.x);
                fflush(stdout);
                exit(1);
            }

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
            if (sliceId < 0 || sliceId >= size.y) {
                printf("Error: Slice %i out of Y range [0, %lu]\n", sliceId, size.y);
                fflush(stdout);
                exit(1);
            }

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
            if (sliceId < 0 || sliceId >= size.z) {
                printf("Error: Slice %i out of Z range [0, %lu]\n", sliceId, size.z);
                fflush(stdout);
                exit(1);
            }

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
