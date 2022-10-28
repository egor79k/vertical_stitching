#ifndef TIFFIMAGE_H
#define TIFFIMAGE_H


template<typename T>
class TiffImage
{
public:
    TiffImage(const size_t width_, const size_t height_);
    ~TiffImage();

    void clear();
    T* data();
    const T* data() const;

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
TiffImage::TiffImage(const size_t width_, const size_t height_) :
    data(new T[width_ * height_]),
    width(width_),
    height(height_) {}


template<typename T>
TiffImage::~TiffImage() {
    clear();
}


template<typename T>
void TiffImage::clear() {
    if (data != nullptr) {
        delete[] data;
    }

    data = nullptr;
    width = 0;
    height = 0;
}


template<typename T>
T *TiffImage::data()
{
    return data;
}


template<typename T>
const T *TiffImage::data() const
{
    return data;
}


template<typename T>
size_t TiffImage::getWidth() const
{
    return width;
}


template<typename T>
size_t TiffImage::getHeight() const
{
    return height;
}


#endif // TIFFIMAGE_H
