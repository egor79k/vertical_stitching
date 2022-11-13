find_package(TinyTIFF REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_library(stitcher
    stitcher.cpp
    stitcher.h
    voxel_container.cpp
    voxel_container.h
    tiff_image.h
    )

target_link_libraries(stitcher TinyTIFF ${OpenCV_LIBS})
