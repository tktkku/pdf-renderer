#pragma once
#include "pdf.h"

struct pdf_render;
typedef struct pdf_renderer pdf_renderer_t;
pdf_render* pdf_render_init(pdf_page_t* page);
void pdf_render_build(pdf_render* render);
void pdf_render_run(pdf_render* render, pdf_renderer_t* renderer);
void pdf_render_free(pdf_render* render);

typedef struct pdf_renderer_vtable
{
    void (*set_color)(pdf_renderer_t* renderer, float r, float g, float b);
    void (*get_color)(pdf_renderer_t* renderer, float *r, float *g, float *b);

    void (*set_fill_rule)(pdf_renderer_t* renderer, bool even_odd);
    void (*set_dash_offset)(pdf_renderer_t* renderer, float offset);
    void (*set_dash_array)(pdf_renderer_t* renderer, float* dashs, int nums);
    /**
        0: JOIN_MITER, ///< Miter join with sharp corners.
        1: JOIN_ROUND, ///< Rounded join.
        2: JOIN_BEVEL ///< Beveled join with a flattened corner.
     */
    void (*set_line_join)(pdf_renderer_t* renderer, int join);
    /**
    0:CAP_BUTT, ///< Flat edge at the end of the stroke.
    1:CAP_ROUND, ///< Rounded ends at the end of the stroke.
    2:CAP_SQUARE 
    */
    void (*set_line_cap)(pdf_renderer_t* renderer, int cap);
    void (*set_miter_limit)(pdf_renderer_t* renderer, float limit);
    void (*set_line_width)(pdf_renderer_t* renderer, float width);

    void (*get_current_point)(pdf_renderer_t* renderer, float *x, float *y);

    void (*scale)(pdf_renderer_t* renderer, float sx, float sy);
    void (*stroke)(pdf_renderer_t* renderer);
    void (*fill)(pdf_renderer_t* renderer);
    void (*close_path)(pdf_renderer_t* renderer);
    void (*new_path)(pdf_renderer_t* renderer);
    void (*cubic_to)(pdf_renderer_t* renderer, float x1, float y1, float x2, float y2, float x3, float y3);
    void (*line_to)(pdf_renderer_t* renderer, float x, float y);
    void (*move_to)(pdf_renderer_t* renderer, float x, float y);
    void (*rect)(pdf_renderer_t* renderer, float x, float y, float width, float height);
    void (*clip)(pdf_renderer_t* renderer);
    
    void (*transform)(pdf_renderer_t* renderer, float a, float b, float c, float d, float e, float f);
    
    void (*save)(pdf_renderer_t* renderer);
    void (*restore)(pdf_renderer_t* renderer);
    void (*draw_image)(pdf_renderer_t* renderer, int width, int height, int channels, unsigned char* pixles, int size);
} pdf_renderer_vtable_t;

struct pdf_renderer {
    const pdf_renderer_vtable_t* vtable;
    void* user_data;
};
#define PDF_RENDERER_CALL(r, m, ...) ((r)->vtable->m((r), ##__VA_ARGS__))