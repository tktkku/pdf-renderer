#ifndef _PDF_H_
#define _PDF_H_
#include "plutovg.h"
#include <stdbool.h>
#include <stddef.h>
#include <map>
#include <stack>
#include <vector>
#include <string>

class PdfArray;
class PdfDict;
struct PdfValue;
class PdfObj;
class PdfPage;
class PdfStream;

typedef struct pdf_file pdf_file_t;
typedef struct pdf_resources pdf_resources_t;

typedef struct pdf_font pdf_font_t;

typedef struct pdf_image pdf_image_t;
typedef struct pdf_form pdf_form_t;
typedef struct pdf_xobject pdf_xobject_t;

class PdfToken;
class PdfParser;

struct pdf_buffer;
typedef struct pdf_buffer pdf_buffer_t;

struct pdf_cmap;
typedef struct pdf_cmap pdf_cmap_t;


pdf_file_t* pdf_file_read_file(const char* file_name);
void pdf_file_free(pdf_file_t* file);
PdfObj* pdf_file_get_obj(pdf_file_t* pdf, int ref);
int pdf_file_get_pages(pdf_file_t* pdf);
PdfPage* pdf_file_get_page(pdf_file_t* pdf, int pageNo);
void pdf_file_free_page(pdf_file_t* pdf, PdfPage* page);
pdf_cmap_t* pdf_file_get_cmap(pdf_file_t* pdf, char* name);

pdf_font_t* pdf_font_init();
void pdf_font_free(pdf_font_t* font);

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
    PdfPage* page;
    PdfObj* current_obj;
    pdf_graphics_state_t* state;
    std::map<int, pdf_font_t*> fontCache;
    std::stack<std::vector<char>> stack;
} pdf_context_t;
void render_to_png_by_plutovg(PdfPage* page, char* filename);
void render_to_buffer_by_plutovg(PdfPage* page, unsigned char* pixels,
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