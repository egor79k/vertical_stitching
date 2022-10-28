find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(stitcher)

include_directories(stitcher)

add_executable(vertical_stitching
    main.cpp
    ui/mainwindow.cpp
    ui/mainwindow.h
    ui/mainwindow.ui
    )

target_link_libraries(vertical_stitching PRIVATE Qt5::Widgets stitcher)
