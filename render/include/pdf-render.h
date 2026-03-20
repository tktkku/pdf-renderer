#pragma once
#include "pdf.h"

struct pdf_render;
pdf_render* pdf_render_init(pdf_page_t* page, int dpi);
pdf_render* pdf_render_init_with_size(pdf_page_t* page, int width, int height, int stride, int dpi);
void pdf_render_do(pdf_render* render);
void pdf_render_free(pdf_render* render);
int pdf_render_copy_to_buffer(pdf_render* context, void* data, int len);
void pdf_render_save_to_png(pdf_render* context, char* filename);
/**
rotation:
0=Auto rotate
1=rotate 0 
2=rotate 90
3=rotate 180
4=rotate 270
*/
pdf_render* pdf_render_init_for_paper(pdf_page_t* page, 
    int paperWidth, int paperHeight, int paperStride, 
    int rotation, int dpi);