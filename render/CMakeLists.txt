project(pdf-render LANGUAGES CXX VERSION 1.0.0)

set(PARSER_SOURCES
    include/pdf.h
    src/pdf-common.cc
    src/pdf-cff.cc
    src/pdf-deque.cc
    src/pdf-array.cc
    src/pdf-dict.cc
    src/pdf-token.cc
    src/pdf-file.cc
    src/pdf-font.cc
    src/pdf-obj.cc
    src/pdf-page.cc
    src/pdf-parser.cc
    src/pdf-stream.cc
    src/pdf-input.cc
    include/pdf-render.h
    src/Do.cc
    src/Tf.cc
    src/color.cc
    src/handle_cff.cc
    src/text.cc
    src/text-render.cc
    src/path.cc
    src/pdf-render.cc
    src/pdf-matrix.cc
)
set(PARSER_DEP zlibstatic plutovg)
if(NOT WIN32)
    set(PARSER_DEP ${PARSER_DEP} m)
endif()
add_library(${PROJECT_NAME} ${PARSER_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PARSER_DEP})
target_include_directories(${PROJECT_NAME} PRIVATE ../include ../plutovg/include ../plutovg/source ../zlib ./CMap)
target_include_directories(${PROJECT_NAME} PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/pdf-render>
)