#pragma once
#include "pdf.h"
typedef void (*PDF_RENDER_FONT_LOAD_CB)(const char* fontname, char** data, long* len);
typedef struct render pdf_render_t;
pdf_render_t* pdf_render_init(pdf_page_t* page);
void pdf_render_do(pdf_render_t* render);
void pdf_render_set_load_font_callback(pdf_render_t* render, PDF_RENDER_FONT_LOAD_CB cb);
void pdf_render_free(pdf_render_t* render);
int pdf_render_copy_to_buffer(pdf_render_t* context, void* data, int len);
void pdf_render_save_to_png(pdf_render_t* context, char* filename);