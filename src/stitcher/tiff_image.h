#ifndef TIFFIMAGE_H
#define TIFFIMAGE_H


#include <tinytiffreader.hxx>
#include <tinytiffwriter.h>
#include <tinytiff_tools.hxx>
#include <opencv2/opencv.hpp>


/**
 * \brief Class for manipulations with monochrome TIFF images of different data types.
 * Work with TIFF files is based on the <a href="https://jkriege2.github.io/TinyTIFF/index.html">TinyTIFF</a> library, so this class supports reading and writing TIFF images with the following data types: int (8, 16, 32 bits), unsigned int (8, 16, 32 bits), float (32 bits).
 */
template<typename T>
class TiffImage
{
public:
    /**
     * \brief Default constructor. Creates an empty image.
     */
    TiffImage() {}

    /**
     * \brief Constructs an image of the given size.
     * 
     * \param[in] width_ Image width
     * \param[in] height_ Image height
     */
    TiffImage(const size_t width_, const size_t height_);

    /**
     * \brief Copy constructor.
     * 
     * \param[in] other Image to copy from
     */
    TiffImage(const TiffImage& other);

    /**
     * \brief Destructor.
     */
    ~TiffImage();

    /**
     * \brief Gives image size by specified file name.
     * 
     * \param[in] fileName Iamge path
     * \param[in] img_width Destination variable for image width
     * \param[in] img_height Destination variable for image height
     * \return True - if success, false - if failed
     */
    static bool getSizeFromFile(const char* fileName, size_t& img_width, size_t& img_height);

    /**
     * \brief Reads image from file.
     * 
     * \param[in] fileName Iamge path
     * \param[in] data Buffer for the image data
     * \param[in] width Destination variable for image width
     * \param[in] height Destination variable for image height
     * \return True - if success, false - if failed.
     */
    static bool readFromFile(const char* fileName, T* data, size_t& width, size_t& height);

    /**
     * \brief Clears image deallocating memory.
     */
    void clear();

    /**
     * \brief Reallocates image of given size.
     * 
     * \param[in] new_width Image width
     * \param[in] new_height Image height
     */
    void resize(const size_t new_width, const size_t new_height);

    /**
     * \brief Writes image in other formats.
     * 
     * \param[in] fileName Image save path with specified format extention. See <a href="https://docs.opencv.org/3.4/d4/da8/group__imgcodecs.html#gabbc7ef1aa2edfaa87772f1202d67e0ce">cv::imwrite</a> for supported formats.
     * \return True - if success, false - if failed.
     */
    bool saveAs(const char* fileName) const;

    /**
     * \brief Writes image in TIFF format.
     * 
     * \param[in] fileName Image save path.
     * \return True - if success, false - if failed.
     */
    bool save(const char* fileName) const;

    /**
     * \brief Writes image in TIFF format from specified memory.
     * 
     * \param[in] fileName Image save path
     * \param[in] data Buffer containing image data
     * \param[in] width Image width
     * \param[in] height Image height
     * \return True - if success, false - if failed.
     */
    static bool save(const char* fileName, T* data, const size_t width, const size_t height);

    /**
     * \brief Gives access to the image data buffer.
     * 
     * \return Pointer to the image data.
     */
    T* getData();

    /**
     * \brief Gives access to the constant image data buffer.
     * 
     * \return Constant pointer to the image data.
     */
    const T* getData() const;

    /**
     * \brief Gives image width.
     * 
     * \return Image width.
     */
    size_t getWidth() const;

    /**
     * \brief Gives image height.
     * 
     * \return Image height.
     */
    size_t getHeight() const;

private:
    T* data = nullptr;
    size_t width = 0;
    size_t height = 0;
};


template<typename T>
TiffImage<T>::TiffImage(const size_t width_, const size_t height_) :
    data(new T[width_ * height_]),
    width(width_),
    height(height_) {}


template<typename T>
TiffImage<T>::TiffImage(const TiffImage& other) {
    if (other.data != nullptr) {
        width = other.width;
        height = other.height;
        data = new T[width * height];
        memcpy(data, other.data, width * height);
    }
}


template<typename T>
TiffImage<T>::~TiffImage() {
    clear();
}


template<typename T>
bool TiffImage<T>::getSizeFromFile(const char* fileName, size_t& img_width, size_t& img_height) {
    TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(fileName);

    if (!tiffr) {
        return false;
    }

    img_width = TinyTIFFReader_getWidth(tiffr);
    img_height = TinyTIFFReader_getHeight(tiffr);

    TinyTIFFReader_close(tiffr);
    return true;
}


template<typename T>
bool TiffImage<T>::readFromFile(const char* fileName, T* data, size_t& width, size_t& height) {
    // Open image
    TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(fileName);

    if (!tiffr) {
        return false;
    }

    width = TinyTIFFReader_getWidth(tiffr);
    height = TinyTIFFReader_getHeight(tiffr);
    uint16_t sformat = TinyTIFFReader_getSampleFormat(tiffr);
    uint16_t bits = TinyTIFFReader_getBitsPerSample(tiffr, 0);

    // Determine the image data type, read it to float type and convert to common range
    switch(sformat) {
        case TINYTIFF_SAMPLEFORMAT_UINT: {
            switch(bits) {
                case 8:
                    TinyTIFFReader_readFrame<uint8_t, T>(tiffr, data);
                    break;
                case 16:
                    TinyTIFFReader_readFrame<uint16_t, T>(tiffr, data);
                    break;
                case 32:
                    TinyTIFFReader_readFrame<uint32_t, T>(tiffr, data);
                    break;
                default:
                    TinyTIFFReader_close(tiffr);
                    return false;
            }
            break;
        }

        case TINYTIFF_SAMPLEFORMAT_INT: {
            switch(bits) {
                case 8:
                    TinyTIFFReader_readFrame<int8_t, T>(tiffr, data);
                    break;
                case 16:
                    TinyTIFFReader_readFrame<int16_t, T>(tiffr, data);
                    break;
                case 32:
                    TinyTIFFReader_readFrame<int32_t, T>(tiffr, data);
                    break;
                default:
                    TinyTIFFReader_close(tiffr);
                    return false;
            }
            break;
        }

        case TINYTIFF_SAMPLEFORMAT_FLOAT: {
            switch(bits) {
                case 32:
                    TinyTIFFReader_readFrame<float, T>(tiffr, data);
                    break;
                default:
                    TinyTIFFReader_close(tiffr);
                    return false;
            }
            break;
        }

        default:
            TinyTIFFReader_close(tiffr);
            return false;
    }

    if (TinyTIFFReader_wasError(tiffr)) {
        TinyTIFFReader_close(tiffr);
        return false;
    }

    TinyTIFFReader_close(tiffr);
    return true;
}


template<typename T>
void TiffImage<T>::clear() {
    if (data != nullptr) {
        delete[] data;
    }

    data = nullptr;
    width = 0;
    height = 0;
}


template<typename T>
void TiffImage<T>::resize(const size_t new_width, const size_t new_height) {
    clear();
    width = new_width;
    height = new_height;
    data = new T[width * height];
}


template<typename T>
bool TiffImage<T>::saveAs(const char *fileName) const {
    cv::Mat_<T> img(height, width, data);
    return cv::imwrite(fileName, img);
}


template<typename T>
bool TiffImage<T>::save(const char *fileName) const {
    return save(fileName, data, width, height);
}


template<typename T>
bool TiffImage<T>::save(const char* fileName, T* data, const size_t width, const size_t height) {
    TinyTIFFWriterFile* tiff = TinyTIFFWriter_open(fileName, sizeof(T) * 8, TinyTIFF_SampleFormatFromType<T>().format, 1, width, height, TinyTIFFWriter_Greyscale);

    if (!tiff) {
        return false;
    }

    TinyTIFFWriter_writeImage(tiff, data);

    TinyTIFFWriter_close(tiff);

    return true;
}


template<typename T>
T *TiffImage<T>::getData()
{
    return data;
}


template<typename T>
const T *TiffImage<T>::getData() const
{
    return data;
}


template<typename T>
size_t TiffImage<T>::getWidth() const
{
    return width;
}


template<typename T>
size_t TiffImage<T>::getHeight() const
{
    return height;
}


#endif // TIFFIMAGE_H
