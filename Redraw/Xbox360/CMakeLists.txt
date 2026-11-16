cmake_minimum_required(VERSION 3.25)
project(redraw_xbox360 CXX)

add_executable(redraw_xbox360
    main.cpp
    XsdkRenderDevice.cpp
    ../../STK/Source/SPMReader.cpp
    ../../STK/Source/SPMRedrawBridge.cpp
    ../../STK/Source/TrackDataReader.cpp
    ../../STK/Source/KartDynamics.cpp
)

set_target_properties(redraw_xbox360 PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
    OUTPUT_NAME "SuperTuxKart360"
)

target_compile_options(redraw_xbox360 PRIVATE -Wall -Wextra -Werror)
target_include_directories(redraw_xbox360 PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../Renderer"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../STK/Source"
)

xsdk_add_shaders(redraw_xbox360 redraw.vert redraw.frag)
xsdk_add_xex(redraw_xbox360
    TITLE_ID 0x53544B36
    NAME "SuperTuxKart360"
    VERSION 0.1.0.0
)
