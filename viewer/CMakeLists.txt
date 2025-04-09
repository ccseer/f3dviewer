cmake_minimum_required(VERSION 3.19)
project(viewer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)

qt_standard_project_setup()

qt_add_executable(viewer
    WIN32 MACOSX_BUNDLE
    test.cpp
    f3dviewer.cpp
    f3dviewer.h
    f3dviewer.json
)

target_link_libraries(viewer
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::OpenGLWidgets
)

# f3d
set(CMAKE_PREFIX_PATH "../F3D/lib/cmake/f3d")
find_package(f3d REQUIRED COMPONENTS library)
target_link_libraries(viewer PRIVATE f3d::libf3d)
include_directories("../F3D/include")

target_sources(viewer
    PRIVATE
        ../src/F3DWidget.cpp
        ../src/F3DWidget.h
)
target_include_directories(viewer PRIVATE ../src)

# seer
target_include_directories(viewer PRIVATE Seer-sdk)

