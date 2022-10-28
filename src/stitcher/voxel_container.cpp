#include <limits>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <tinytiffreader.hxx>
#include "voxel_container.h"


size_t VoxelContainer::Vector3::volume() {
    return x * y * z;
}


VoxelContainer::VoxelContainer(const std::vector<std::string>& fileNames) {
    loadFromImages(fileNames);
}


VoxelContainer::VoxelContainer(float* _data, const Vector3& _size) :
    data(_data),
    size(_size) {}


VoxelContainer::~VoxelContainer() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
        size = {0, 0, 0};
    }
}


void VoxelContainer::fitToFloatRange(int64_t min, int64_t max) {
    for (int z = 0; z < size.z; ++z) {
        for (int y = 0; y < size.y; ++y) {
            for (int x = 0; x < size.x; ++x) {
                int id = z * size.x * size.y + y * size.x + x;
                data[id] = (data[id] - min) / (max - min) * 255;
            }
        }
    }
}


template<typename Tin>
bool VoxelContainer::readImagesToFloat(const std::vector<std::string>& fileNames) {
    for (int i = 0; i < size.z; ++i) {
        // Open image
        TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(fileNames.at(i).data());

        if (!tiffr) {
            // QMessageBox::information(0, "Reading error", "Failed to load image " + fileNames.at(i));
            return false;
        }

        uint32_t width = TinyTIFFReader_getWidth(tiffr);
        uint32_t height = TinyTIFFReader_getHeight(tiffr);

        // Check that current image size is the same
        if (width != size.x || height != size.y) {
            // QMessageBox::information(0, "Reading error", "Different size of image " + fileNames.at(i));
            return false;
        }

        // Read iamge data to current slice
        TinyTIFFReader_readFrame<Tin, float>(tiffr, data + i * size.x * size.y);

        if (TinyTIFFReader_wasError(tiffr)) {
            // QMessageBox::information(0, "Reading error", QObject::tr("error reading\n"));
            return false;
        }

        TinyTIFFReader_close(tiffr);
    }

    return true;
}


bool VoxelContainer::loadFromImages(const std::vector<std::string>& fileNames) {
    clear();
    
    // Open first image
    TinyTIFFReaderFile* tiffr = TinyTIFFReader_open(fileNames.front().data());

    if (!tiffr) {
        // QMessageBox::information(0, "Reading error", "Failed to load image " + fileNames.first());
        return false;
    }

    // Read first image parameters
    size.x = TinyTIFFReader_getWidth(tiffr);
    size.y = TinyTIFFReader_getHeight(tiffr);
    size.z = fileNames.size();

    uint16_t sformat = TinyTIFFReader_getSampleFormat(tiffr);
    uint16_t bits = TinyTIFFReader_getBitsPerSample(tiffr, 0);

    TinyTIFFReader_close(tiffr);

    data = new float[size.volume()];

    // Determine the image data type, read it to float type and convert to common range
    switch(sformat) {
        case TINYTIFF_SAMPLEFORMAT_UINT: {
            switch(bits) {
                case 8:
                    readImagesToFloat<uint8_t>(fileNames);
                    break;
                case 16:
                    readImagesToFloat<uint16_t>(fileNames);
                    break;
                case 32:
                    readImagesToFloat<uint32_t>(fileNames);
                    break;
                default:
                    // QMessageBox::information(0, "Reading error", QObject::tr("datatype not convertible to float (type=%1, bitspersample=%2)\n").arg(sformat).arg(bits));
                    return false;
            }

            fitToFloatRange(0, 1 << bits - 1);
            break;
        }

        case TINYTIFF_SAMPLEFORMAT_INT: {
            switch(bits) {
                case 8:
                    readImagesToFloat<int8_t>(fileNames);
                    break;
                case 16:
                    readImagesToFloat<int16_t>(fileNames);
                    break;
                case 32:
                    readImagesToFloat<int32_t>(fileNames);
                    break;
                default:
                    // QMessageBox::information(0, "Reading error", QObject::tr("datatype not convertible to float (type=%1, bitspersample=%2)\n").arg(sformat).arg(bits));
                    return false;
            }

            int64_t range_val = 1 << (bits - 1) - 1;
            fitToFloatRange(-range_val, range_val);
            break;
        }

        case TINYTIFF_SAMPLEFORMAT_FLOAT: {
            switch(bits) {
                case 32:
                    readImagesToFloat<float>(fileNames);
                    break;
                default:
                    // QMessageBox::information(0, "Reading error", QObject::tr("datatype not convertible to float (type=%1, bitspersample=%2)\n").arg(sformat).arg(bits));
                    return false;
            }

            fitToFloatRange(-1, 1);
            break;
        }

        default:
            // QMessageBox::information(0, "Reading error", QObject::tr("datatype not convertible to float (type=%1, bitspersample=%2)\n").arg(sformat).arg(bits));
            return false;
    }

    return true;
}


bool VoxelContainer::loadFromJson(const std::string& fileName) {
    // Open parameters file
    QFile file(QString::fromStdString(fileName));

    if(!file.open(QIODevice::ReadOnly)) {
        // QMessageBox::information(0, "File opening error", file.errorString());
        return false;
    }

    // Parse JSON parameters
    QJsonParseError error;
    QJsonObject json = QJsonDocument::fromJson(file.readAll(), &error).object();

    if (error.error) {
        // qDebug() << "Error: " << error.errorString() << error.offset << error.error;
        // QMessageBox::information(0, "JSON parsing error", error.errorString());
        return false;
    }

    int height = json["height"].toInt();
    std::string format = json["format"].toString().toStdString();
    std::string imgPath = fileName.substr(0, fileName.find_last_of('/') + 1);
    std::vector<std::string> imgNames;

    // Create image files list
    for (int i = 0; i < height; ++i) {
        imgNames.push_back(imgPath + std::to_string(i) + format);
    }

    return loadFromImages(imgNames);
}


void VoxelContainer::clear() {
    if (data != nullptr) {
        delete[] data;
        data = nullptr;
        size = {0, 0, 0};
    }
}


bool VoxelContainer::isEmpty() {
    return data == nullptr;
}


const VoxelContainer::Vector3& VoxelContainer::getSize() const {
    return size;
}


const float* VoxelContainer::getData() const {
    return data;
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
