project(render LANGUAGES CXX)

set(RENDER_SOURCES
    Do.cc
    Tf.cc
    color.cc
    handle_cff.cc
    text.cc
    text-render.cc
    path.cc
    pdf-render-by-plutovg.cc
    pdf-render.h
)
set(RENDER_DEP pdf-parser plutovg)
add_library(${PROJECT_NAME} ${RENDER_SOURCES})
target_link_libraries(${PROJECT_NAME} ${RENDER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../plutovg/include ../plutovg/source ../parser ../zlib)