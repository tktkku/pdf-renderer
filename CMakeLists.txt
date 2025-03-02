cmake_minimum_required(VERSION 3.10)

project(pdf-renderer VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_BUILD_TYPE Debug)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(parser)
add_subdirectory(plutovg)
find_package(ZLIB REQUIRED)
add_executable(test test.c)
target_link_libraries(test pdf-parser ${ZLIB_LIBRARIES} plutovg -static)
target_include_directories(test PRIVATE ./parser ./plutovg/include ./plutovg/source)