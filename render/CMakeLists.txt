project(render LANGUAGES C)

set(RENDER_SOURCES
    Do.c
    Tf.c
    pdf-stack.c
    pdf-render-by-plutovg.c
    render.h
)
set(RENDER_DEP pdf-parser plutovg)
add_library(${PROJECT_NAME} ${RENDER_SOURCES})
target_link_libraries(${PROJECT_NAME} ${RENDER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../plutovg/include ../plutovg/source ../parser)