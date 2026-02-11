project(pdf-parser LANGUAGES CXX)

set(PARSER_SOURCES
    pdf-array.cc
    pdf-common.cc
    pdf-cff.cc
    pdf-cmap.cc
    pdf-deque.cc
    pdf-dict.cc
    pdf-file.cc
    pdf-font.cc
    pdf-obj.cc
    pdf-page.cc
    pdf-parser.cc
    pdf-stream.cc
    pdf-input.cc
    pdf-private.h
    pdf.h
)
set(PARSER_DEP zlibstatic)
if(NOT WIN32)
    set(PARSER_DEP ${PARSER_DEP} m)
endif()
add_library(${PROJECT_NAME} ${PARSER_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PARSER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../plutovg/include ../plutovg/source ../zlib ./CMap)