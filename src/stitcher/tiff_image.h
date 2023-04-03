#ifndef TIFFIMAGE_H
#define TIFFIMAGE_H


#include <tinytiffreader.hxx>
#include <tinytiffwriter.h>
#include <tinytiff_tools.hxx>
#include <opencv2/opencv.hpp>


template<typename T>
class TiffImage
{
public:
    TiffImage() {}
    TiffImage(const size_t width_, const size_t height_);
    TiffImage(const TiffImage& other);
    ~TiffImage();

    static bool getSizeFromFile(const char* fileName, size_t& img_width, size_t& img_height);
    static bool readFromFile(const char* fileName, T* data, size_t& width, size_t& height);

    void clear();
    void resize(const size_t new_width, const size_t new_height);
    bool saveAs(const char* fileName) const;
    bool save(const char* fileName) const;
    static bool save(const char* fileName, T* data, const size_t width, const size_t height);

    T* getData();
    const T* getData() const;

    size_t getWidth() const;
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
