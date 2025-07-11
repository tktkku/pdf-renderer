cmake_minimum_required(VERSION 3.10)
project(pdf-renderer VERSION 0.0.1 LANGUAGES CXX)
# 设置统一产物输出路径
set(GLOBAL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})

if(NOT MSVC)
    add_compile_options(
        -Wall 
        -Wno-unused-variable 
        -Wno-int-conversion 
        -Wno-unused-function 
        -Wno-pointer-sign 
        -Wno-unused-but-set-variable 
        -Wno-unused-private-field)
else()
    set(CMAKE_C_FLAGS_RELEASE "/MT")
    set(CMAKE_C_FLAGS_DEBUG "/MTd")
    set(CMAKE_CXX_FLAGS_RELEASE "/MT")
    set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
endif()
set(BUILD_SHARED_LIBS OFF)
set(LIBUSB_BUILD_SHARED_LIBS ON)
add_subdirectory(c-vector)
add_subdirectory(plutovg)
add_subdirectory(zlib)
add_subdirectory(parser)
add_subdirectory(render)
add_subdirectory(ippd)
add_subdirectory(libusb-cmake)

add_executable(test test.cpp)
target_link_libraries(test render)
target_include_directories(test PRIVATE ./parser ./render)
