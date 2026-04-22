cmake_minimum_required(VERSION 3.16)
project(f3dviewer VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# MSVC: Enable Debug Information in Release builds
if(MSVC)
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    add_link_options(
        "$<$<CONFIG:Release>:/DEBUG>"
        "$<$<CONFIG:Release>:/OPT:REF>"
        "$<$<CONFIG:Release>:/OPT:ICF>"
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Svg)

include(FetchContent)
FetchContent_Declare(SeerSdk
    GIT_REPOSITORY https://github.com/ccseer/Seer-sdk.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SeerSdk)

# F3D SDK
set(CMAKE_PREFIX_PATH "../F3D/lib/cmake/f3d")
find_package(f3d REQUIRED COMPONENTS library)
include_directories("../F3D/include")

add_library(f3dviewer SHARED
    f3dviewer.cpp
    f3dviewer.h
    sidebarwnd.cpp
    sidebarwnd.h
    sidebarwnd.ui
    f3dwidget/F3DWidget.cpp
    f3dwidget/F3DWidget.h
    ${seersdk_SOURCE_DIR}/seer/viewerbase.h
    bin/plugin.json
)

target_link_libraries(f3dviewer PRIVATE
    SeerSdk::SeerSdk
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
    Qt6::Svg
    f3d::libf3d
)

set_target_properties(f3dviewer PROPERTIES
    OUTPUT_NAME "f3dviewer"
    SUFFIX ".dll"
)

target_compile_definitions(f3dviewer PRIVATE
    COMPANY_NAME="1218.io"
    PRODUCT_NAME="Seer"
)

add_executable(f3dviewer_test WIN32 test.cpp)
target_link_libraries(f3dviewer_test PRIVATE
    f3dviewer
    SeerSdk::SeerSdk
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets
)
