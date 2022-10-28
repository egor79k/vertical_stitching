#ifndef TIFFIMAGE_H
#define TIFFIMAGE_H
#include <iostream>

template<typename T>
class TiffImage
{
public:
    TiffImage() {}
    TiffImage(const size_t width_, const size_t height_);
    TiffImage(const TiffImage& other);
    ~TiffImage();

    void clear();
    void resize(const size_t new_width, const size_t new_height);

    T* getData();
    const T* getData() const;

    size_t getWidth() const;
    size_t getHeight() const;

private:
    T* data = nullptr;
    size_t width = 0;
    size_t height = 0;
//    T range_min = 0;
//    T range_max = 0;
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
    printf("Copied\n");
}


template<typename T>
TiffImage<T>::~TiffImage() {
    clear();
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
