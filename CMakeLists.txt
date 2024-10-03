cmake_minimum_required(VERSION 3.10)
project(Finder LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if (NOT MSVC)
    message(FATAL_ERROR "msvc is required")
endif ()

# dll
add_library(${PROJECT_NAME} SHARED)

# include dirs
include_directories(include include/sqlite include/SQLiteCpp include/nlohmann include/thread_pool src res)
include_directories($ENV{OPENSSL_INCLUDE_DIR})

# src
file(GLOB SOURCES src/*.cpp)
file(GLOB IMGUI_SRC include/imgui/*.cpp)
file(GLOB SQLITE_CPP_SRC include/SQLiteCpp/*.cpp)

# add source files
target_sources(
        ${PROJECT_NAME}
        PRIVATE
        ${SOURCES}
        ${IMGUI_SRC}
        ${SQLITE_CPP_SRC}
        ${RESOURCE_FILES}
)

# resources
file(GLOB_RECURSE RESOURCE_IMAGES res/*.png)

foreach (IMAGE ${RESOURCE_IMAGES})
    configure_file(${IMAGE} ${CMAKE_BINARY_DIR}/res COPYONLY)
endforeach ()

# add resources
target_sources(
        ${PROJECT_NAME}
        PRIVATE
        res/resource.h
        res/resource.rc
        ${RESOURCE_IMAGES}
)

# define openssl lib based on build config
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OPENSSL_LIB_PATH $ENV{OPENSSL_LIB_DIR}/VC/x64/MTd)
    # /MTd required for compatibility
    target_compile_options(${PROJECT_NAME} PRIVATE /MTd /Zi /Od)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /DEBUG")
else ()
    set(OPENSSL_LIB_PATH $ENV{OPENSSL_LIB_DIR}/VC/x64/MT)
    # /MT required for compatibility
    target_compile_options(${PROJECT_NAME} PRIVATE /MT /O2)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif ()

# link openssl static
target_link_libraries(
        ${PROJECT_NAME}
        PRIVATE
        ${OPENSSL_LIB_PATH}/libssl_static.lib
        ${OPENSSL_LIB_PATH}/libcrypto_static.lib
)

# include openssl
target_include_directories(${PROJECT_NAME} PRIVATE $ENV{OPENSSL_INCLUDE_DIR})

# link sqlite3 static
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/sqlite/sqlite3.lib)
