project(pdf-parser LANGUAGES CXX)

set(PARSER_SOURCES
    MemoryPool.cpp
    pdf-array.cpp
    pdf-common.cpp
    pdf-cmap.cpp
    pdf-dict.cpp
    pdf-file.cpp
    pdf-font.cpp
    pdf-obj.cpp
    pdf-render-by-plutovg.cpp
    pdf-page.cpp
    pdf-parser.cpp
    pdf-stream.cpp
    pdf-private.h
    pdf.h
)
set(PARSER_DEP zlibstatic plutovg)
if(NOT WIN32)
    set(PARSER_DEP ${PARSER_DEP} m)
endif()
add_library(${PROJECT_NAME} ${PARSER_SOURCES})
target_link_libraries(${PROJECT_NAME} ${PARSER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../plutovg/include ../plutovg/source ../zlib)