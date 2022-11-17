find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(stitcher)

add_subdirectory(stitcher)

add_executable(vertical_stitching
    main.cpp
    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/mainwindow.ui
    )

target_link_libraries(vertical_stitching PRIVATE Qt5::Widgets stitcher)
