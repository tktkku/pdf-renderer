cmake_minimum_required(VERSION 3.10)

project(pdf-renderer VERSION 0.0.1 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(parser)
add_subdirectory(plutovg)
add_subdirectory(zlib)

add_executable(test test.cpp)
target_link_libraries(test pdf-parser)
target_include_directories(test PRIVATE ./parser)