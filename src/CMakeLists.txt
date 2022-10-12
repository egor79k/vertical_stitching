find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(TinyTIFF REQUIRED)

add_executable(vertical_stitching
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    stitcher.cpp
    stitcher.h
    voxel_container.cpp
    voxel_container.h
    )

target_link_libraries(vertical_stitching PRIVATE Qt5::Widgets TinyTIFF)
