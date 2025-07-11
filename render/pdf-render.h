#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "pdf.h"

typedef struct render pdf_render_t;
pdf_render_t* pdf_render_init(pdf_page_t* page, int dpi);
pdf_render_t* pdf_render_init_with_size(pdf_page_t* page, int width, int height, int stride, int dpi);
void pdf_render_do(pdf_render_t* render);
void pdf_render_free(pdf_render_t* render);
int pdf_render_copy_to_buffer(pdf_render_t* context, void* data, int len);
void pdf_render_save_to_png(pdf_render_t* context, char* filename);
#ifdef __cplusplus
}
#endif