cmake_minimum_required(VERSION 3.10)
project(pdf-renderer VERSION 0.0.1 LANGUAGES C CXX)
# 设置统一产物输出路径
set(GLOBAL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/output)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_DIR})

if(NOT MSVC)
    add_compile_options(
        -Wall 
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable)
else()
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd /Od /Zi")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Od /Zi")
endif()
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(plutovg)
add_subdirectory(zlib)
add_subdirectory(render)

add_executable(test test.cpp)
target_link_libraries(test pdf-render plutovg)
target_include_directories(test PRIVATE ./render/include ./plutovg/include)
