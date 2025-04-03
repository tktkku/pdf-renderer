cmake_minimum_required(VERSION 3.10)

project(pdf-renderer VERSION 0.0.1 LANGUAGES C CXX)
add_compile_options(-Wall)
set(CMAKE_BUILD_TYPE Debug)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(c-vector)
add_subdirectory(plutovg)
add_subdirectory(zlib)
add_subdirectory(parser)
add_subdirectory(render)

add_executable(test test.c)
target_link_libraries(test render)
target_include_directories(test PRIVATE ./parser)