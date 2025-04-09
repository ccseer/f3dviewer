cmake_minimum_required(VERSION 3.19)
project(F3DViewer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)

qt_standard_project_setup()

qt_add_executable(F3DViewer
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/F3DWidget.cpp
    src/F3DWidget.h
)
target_link_libraries(F3DViewer
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::OpenGLWidgets
)

# f3d
set(CMAKE_PREFIX_PATH "./F3D/lib/cmake/f3d")
find_package(f3d REQUIRED COMPONENTS library)
target_link_libraries(F3DViewer PRIVATE f3d::libf3d)
include_directories("./F3D/include")
