cmake_minimum_required(VERSION 3.10)
project(MockStandalone LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if (NOT MSVC)
    message(FATAL_ERROR "msvc is required")
endif ()

add_executable(${PROJECT_NAME} WIN32)

# include imgui
include_directories(../include/imgui ../include/imgui/backends)

# add imgui sources
file(GLOB IMGUI_SRC ../include/imgui/*.cpp)
set(IMGUI_BACKEND_SRC ../include/imgui/backends/imgui_impl_dx11.cpp ../include/imgui/backends/imgui_impl_win32.cpp)

# sources
target_sources(
        ${PROJECT_NAME}
        PRIVATE
        ${IMGUI_SRC}
        ${IMGUI_BACKEND_SRC}
        stb_image.h
        main.cpp
)

# build config
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE /Zi /Od)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DEBUG")
else ()
    target_compile_options(${PROJECT_NAME} PRIVATE /O2)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif ()

# link with dx11
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        d3dcompiler.lib
        d3d11.lib
        dxgi.lib
)
