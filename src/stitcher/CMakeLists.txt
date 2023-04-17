find_package(TinyTIFF REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_library(stitcher
    stitcher.cpp
    separation_stitcher.cpp
    direct_alignment_stitcher.cpp
    opencv_sift_2d_stitcher.cpp
    sift_2d_stitcher.cpp
    sift_3d_stitcher.cpp
    voxel_container.cpp
    )

target_link_libraries(stitcher TinyTIFF ${OpenCV_LIBS})
