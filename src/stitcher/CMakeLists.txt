find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(TinyTIFF REQUIRED)

add_library(stitcher
    stitcher.cpp
    stitcher.h
    voxel_container.cpp
    voxel_container.h
    tiff_image.h
    )

target_link_libraries(stitcher PRIVATE Qt5::Widgets TinyTIFF)
