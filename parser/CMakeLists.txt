project(pdf-parser LANGUAGES C)

set(PARSER_SOURCES
    pdf-array.c
    pdf-common.c
    pdf-cmap.c
    pdf-dict.c
    pdf-file.c
    pdf-font.c
    pdf-obj.c
    pdf-render-by-plutovg.c
    pdf-page.c
    pdf-parser.c
    pdf-stack.c
    pdf-stream.c
    pdf-private.h
    pdf.h
)
# set(PARSER_DEP zlib)
# if(NOT WIN32)
#     set(PARSER_DEP ${PARSER_DEP} m)
# endif()
add_library(${PROJECT_NAME} ${PARSER_SOURCES})
target_link_libraries(${PROJECT_NAME} ${PARSER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../plutovg/include ../plutovg/source)