cmake_minimum_required(VERSION 3.10)
project(Finder LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

if (NOT MSVC)
    message(FATAL_ERROR "msvc is required")
endif ()

add_library(${PROJECT_NAME} SHARED)

# include dirs
include_directories(include include/imgui include/sqlite include/SQLiteCpp include/nlohmann include/thread_pool src res)
include_directories($ENV{OPENSSL_INCLUDE_DIR})

# src
file(GLOB_RECURSE SOURCES src/*.cpp)
#file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE SQLITECPP_SRC include/SQLiteCpp/*.cpp)
# resources
file(GLOB_RECURSE RESOURCE_FILES res/*.rc)
file(GLOB_RECURSE RESOURCE_IMAGES res/*.png)

foreach (IMAGE ${RESOURCE_IMAGES})
    configure_file(${IMAGE} ${CMAKE_BINARY_DIR}/res COPYONLY)
endforeach ()

# embed resources
target_sources(${PROJECT_NAME} PRIVATE ${RESOURCE_FILES})

target_sources(
        ${PROJECT_NAME}
        PRIVATE
        ${SOURCES}
        #        ${HEADERS}
        ${SQLITECPP_SRC}
        ${RESOURCE_FILES}
)
target_sources(${PROJECT_NAME} PRIVATE res/resource.h)

# link sqlite3 static
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/sqlite/sqlite3.lib)

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
target_link_libraries(${PROJECT_NAME} PRIVATE
        ${OPENSSL_LIB_PATH}/libssl_static.lib
        ${OPENSSL_LIB_PATH}/libcrypto_static.lib)

# export symbol
target_compile_definitions(${PROJECT_NAME} PRIVATE FINDER_EXPORTS)

# include openssl
target_include_directories(${PROJECT_NAME} PRIVATE $ENV{OPENSSL_INCLUDE_DIR})
