cmake_minimum_required(VERSION 3.19)
project(f3dviewer LANGUAGES CXX)

set(BUILD_AS_PLUGIN ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Core Widgets OpenGLWidgets)

qt_standard_project_setup()

set(VIEWER_SOURCES
    f3dviewer.cpp
    f3dviewer.h
    sidebarwnd.ui
    sidebarwnd.cpp
    sidebarwnd.h
    bin/plugin.json
)

if(BUILD_AS_PLUGIN)
    qt6_add_plugin(${PROJECT_NAME} SHARED ${VIEWER_SOURCES})
else()
    qt_add_executable(${PROJECT_NAME} WIN32 ${VIEWER_SOURCES} test.cpp)
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::OpenGLWidgets
)

# f3d
set(CMAKE_PREFIX_PATH "../F3D/lib/cmake/f3d")
find_package(f3d REQUIRED COMPONENTS library)
target_link_libraries(${PROJECT_NAME} PRIVATE f3d::libf3d)
include_directories("../F3D/include")

target_sources(${PROJECT_NAME}
    PRIVATE
        ../src/F3DWidget.cpp
        ../src/F3DWidget.h
)
target_include_directories(${PROJECT_NAME} PRIVATE ../src)

# seer
target_include_directories(${PROJECT_NAME} PRIVATE Seer-sdk)
target_sources(${PROJECT_NAME}
    PRIVATE
        Seer-sdk/seer/viewerbase.h
        Seer-sdk/seer/viewoption.h
)
