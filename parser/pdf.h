#ifndef _PDF_H_
#define _PDF_H_
#include "plutovg.h"
#include <stdbool.h>
#include <stddef.h>
#include <map>
#include <stack>
#include <vector>
#include <string>

//typedef struct pdf_parser_token pdf_parser_token_t;

// typedef struct pdf_value pdf_obj_value_t;
// typedef struct pdf_value pdf_dict_pair_value_t;
// typedef struct pdf_value pdf_array_element_value_t;

struct pdf_obj;
typedef struct pdf_obj pdf_obj_t;

struct pdf_stream;
typedef struct pdf_stream pdf_stream_t;

class PdfArray;
class PdfDict;
struct PdfValue;

typedef struct pdf_file pdf_file_t;
typedef struct pdf_resources pdf_resources_t;
typedef struct pdf_page pdf_page_t;
typedef struct pdf_font pdf_font_t;
typedef struct pdf_stream pdf_stream_t;

typedef struct pdf_image pdf_image_t;
typedef struct pdf_form pdf_form_t;
typedef struct pdf_xobject pdf_xobject_t;

class PdfToken;
class PdfParser;

struct pdf_buffer;
typedef struct pdf_buffer pdf_buffer_t;

struct pdf_cmap;
typedef struct pdf_cmap pdf_cmap_t;

pdf_page_t* pdf_page_init();
void pdf_page_free(pdf_page_t* page);
int pdf_page_get_media_width(pdf_page_t* page);
int pdf_page_get_media_height(pdf_page_t* page);
pdf_font_t* pdf_page_get_font(pdf_page_t* page, const char* name);

void pdf_page_get_ext_gstate(pdf_page_t* page, const char* name);

pdf_xobject_t* pdf_obj_get_xobject(pdf_obj_t* obj);
void pdf_page_xobject_free(pdf_xobject_t* xobject);

int pdf_page_get_streams(pdf_page_t* page);
pdf_stream_t* pdf_page_get_stream(pdf_page_t* page, int index);

void pdf_stream_close(pdf_stream_t* stream);
PdfToken* pdf_stream_get_next_token(pdf_stream_t* stream);
int pdf_stream_get_data(pdf_stream_t* stream, unsigned char* buf, int size);
pdf_stream_t* pdf_stream_init(pdf_file_t* pdf, pdf_obj_t* obj, int len, int offset);
void pdf_stream_free(pdf_stream_t* stream);
void pdf_stream_open(pdf_stream_t* stream);
void pdf_stream_get_all(pdf_stream_t* stream, unsigned char** buffer, int* size);

pdf_obj_t* pdf_obj_init();
void pdf_obj_free(pdf_obj_t* obj);

pdf_file_t* pdf_file_read_file(const char* file_name);
void pdf_file_free(pdf_file_t* file);
pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, int ref);
int pdf_file_get_pages(pdf_file_t* pdf);
pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo);
pdf_cmap_t* pdf_file_get_cmap(pdf_file_t* pdf, char* name);

pdf_font_t* pdf_font_init();
void pdf_font_free(pdf_font_t* font);

// PdfArray* pdf_array_init();
// void pdf_array_free(PdfArray* array);

pdf_cmap_t* pdf_cmap_init();
void pdf_cmap_free(pdf_cmap_t* cmap);
typedef struct pdf_graphics_state {
    char currentColorSpace[256];
    double fillColor[3];
    double strokeColor[3];
    double lineWidth;
    int lineCap;
    int lineJoin;
    double miterLimit;
    struct {
        double* dashs;
        int dash_size;
        double offset;
    } dashPattern;
    struct {
        double characterSpacing;
        double wordSpacing;
        double horizontalScaling;
        double textLeading;
        double fontSize;
        int textMode;
        double textRise;
        plutovg_font_face_t* fontface;
        bool font_face_loaded;
        pdf_font_t* font;
        double textLineWidth;
    } textState;
    struct pdf_graphics_state* next;
} pdf_graphics_state_t;
typedef struct context
{
    //pdf_stack_t* stack;
    plutovg_canvas_t* canvas;
    pdf_file_t* pdf;
    pdf_page_t* page;
    pdf_obj_t* current_obj;
    pdf_graphics_state_t* state;
    std::map<int, pdf_font_t*> fontCache;
    std::stack<std::vector<char>> stack;
} pdf_context_t;
void render_to_png_by_plutovg(pdf_page_t* page, char* filename);
void render_to_buffer_by_plutovg(pdf_page_t* page, unsigned char* pixels,
    int width, int height, int stride);
#define DPI (203)
#ifdef DPI
#define PIXELS_PER_POINT                                                       \
    (DPI / 72.0) // 1point=1/72inch->72point/1inch dots/inch / point/inch
#define POINTS_PER_PIXEL (72.0 / DPI) // point/inch / pixels/inch
#define MM_PER_PIXEL (25.4 / DPI)     // 1inch=25.4mm  mm/inch / dots/inch
#define PIXELS_PER_MM (DPI / 25.4)    // dots/inch / mm/inch
#else
#define PIXELS_PER_POINT(dpi) ((dpi) / 72.0)
#define POINTS_PER_PIXEL(dpi) (72.0 / (dpi))
#define MM_PER_PIXEL(dpi) (25.4 / (dpi))
#define PIXELS_PER_MM(dpi) ((dpi) / 25.4)
#endif
#endif