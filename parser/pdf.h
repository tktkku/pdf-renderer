#ifndef _PDF_H_
#define _PDF_H_

#include <stdbool.h>
#include <stddef.h>
#include "plutovg.h"
//#define CVECTOR_LINEAR_GROWTH
#include "../c-vector/cvector.h"
typedef struct pdf_parser_token pdf_parser_token_t;
typedef enum pdf_parser_token_type pdf_parser_token_type_t;

struct pdf_value;
typedef struct pdf_value pdf_obj_value_t;
typedef struct pdf_value pdf_dict_pair_value_t;
typedef struct pdf_value pdf_array_element_value_t;

struct pdf_obj;
typedef struct pdf_obj pdf_obj_t;

struct pdf_stream;
typedef struct pdf_stream pdf_stream_t;

struct pdf_dict;
typedef struct pdf_dict pdf_dict_t;
typedef struct pdf_dict_pair pdf_dict_pair_t;

struct pdf_array;
typedef struct pdf_array pdf_array_t;

typedef struct pdf_file pdf_file_t;
//typedef struct pdf_resources pdf_resources_t;
typedef struct pdf_page pdf_page_t;
typedef struct pdf_font pdf_font_t;
typedef struct pdf_stream pdf_stream_t;

typedef struct pdf_image pdf_image_t;
typedef enum xobject_type xobject_type_t;
typedef struct pdf_form pdf_form_t;
typedef struct pdf_xobject pdf_xobject_t;

struct pdf_parser;
typedef struct pdf_parser pdf_parser_t;
typedef enum pdf_parser_reader_type pdf_parser_reader_type_t;

struct pdf_buffer;
typedef struct pdf_buffer pdf_buffer_t;

struct pdf_cmap;
typedef struct pdf_cmap pdf_cmap_t;

typedef struct pdf_stack_node pdf_stack_node_t;
typedef struct pdf_stack pdf_stack_t;
void pdf_value_free(struct pdf_value* value);

pdf_stack_t* pdf_stack_init(void);
void pdf_stack_push(pdf_stack_t* s, const void* data, size_t size);
void pdf_stack_pop(pdf_stack_t* s, pdf_stack_node_t* data);
void pdf_stack_free(pdf_stack_t* s);
void pdf_stack_show(pdf_stack_t* s);

/**
 * buf: start position
 * size: size of the buffer
 * user_data: custom param
 * return: actual size that filled in buf
 */
typedef void (*pdf_parser_read_func)(pdf_parser_t* parser, void* source);
/**
 * @param pdf
 * @param type BUFFER_READER, FILE_READER, STREAM_READER
 * @param source pdf_buffer_t*, FILE*, pdf_stream_t*
 */
pdf_parser_t* pdf_parser_init(pdf_file_t* pdf, pdf_parser_reader_type_t type, void* source);
void pdf_parser_free(pdf_parser_t* parser);
pdf_parser_token_t* pdf_parser_token_init(pdf_parser_t* parser, const unsigned char* start, pdf_parser_token_type_t type, int len);
const char* pdf_parser_token_get_token(pdf_parser_token_t* token);
void pdf_parser_token_free(pdf_parser_t* parser, pdf_parser_token_t* token);
pdf_parser_token_t* pdf_parser_next_token(pdf_parser_t* parser);
pdf_obj_t* pdf_parser_build_obj(pdf_parser_t* parser);
pdf_dict_t* pdf_parser_build_dict(pdf_parser_t* parser);
pdf_array_t* pdf_parser_build_array(pdf_parser_t* parser);
pdf_cmap_t* pdf_parser_build_cmap(pdf_parser_t* parser);

pdf_page_t* pdf_page_init(void);
void pdf_page_free(pdf_page_t* page);
int pdf_page_get_media_width(pdf_page_t* page);
int pdf_page_get_media_height(pdf_page_t* page);
pdf_font_t* pdf_page_get_font(pdf_page_t* page, const char* name);

pdf_xobject_t* pdf_obj_get_xobject(pdf_obj_t* obj, const char* name);

int pdf_page_get_streams(pdf_page_t* page);
pdf_stream_t* pdf_page_get_stream(pdf_page_t* page, int index);

void pdf_stream_close(pdf_stream_t* stream);
pdf_parser_token_t* pdf_stream_get_next_token(pdf_stream_t* stream);
int pdf_stream_get_data(pdf_stream_t* stream, unsigned char* buf, int size);
pdf_stream_t* pdf_stream_init(pdf_file_t* pdf, pdf_obj_t* obj, int len, int offset);
void pdf_stream_free(pdf_stream_t* stream);
void pdf_stream_open(pdf_stream_t* stream);
void pdf_stream_get_all(pdf_stream_t* stream, unsigned char** buffer, int* size);

pdf_obj_t* pdf_obj_init(void);
void pdf_obj_free(pdf_obj_t* obj);
pdf_font_t* pdf_obj_get_font(pdf_obj_t* obj, const char* name);

pdf_file_t* pdf_file_read_file(const char* file_name);
void pdf_file_free(pdf_file_t* file);
pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, int ref);
int pdf_file_get_pages(pdf_file_t* pdf);
pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo);
pdf_cmap_t* pdf_file_get_cmap(pdf_file_t* pdf, char* name);

pdf_dict_t* pdf_dict_init(void);
void pdf_dict_free(pdf_dict_t* dict);
double pdf_dict_get_number(pdf_dict_t* dict, const char* name);
int pdf_dict_get_ref(pdf_dict_t* dict, const char* name);
pdf_array_t* pdf_dict_get_array(pdf_dict_t* dict, const char* name);
pdf_dict_t* pdf_dict_get_dict(pdf_dict_t* dict, const char* name);
const char* pdf_dict_get_name(pdf_dict_t* dict, const char* name);
int pdf_dict_get_bool(pdf_dict_t* dict, const char* name);
bool pdf_dict_add_array(pdf_dict_t* dict, const char* name, pdf_array_t* array);
const char* pdf_dict_get_string(pdf_dict_t* dict, const char* name);

pdf_font_t* pdf_font_init(void);
void pdf_font_free(pdf_font_t* font);

pdf_array_t* pdf_array_init(void);
void pdf_array_free(pdf_array_t* array);

pdf_cmap_t* pdf_cmap_init(void);
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
typedef struct
{
    int ref;
    pdf_font_t* font;
    plutovg_font_face_t* fontface;
} pdf_font_cache_t;
typedef struct context
{
    pdf_stack_t* stack;
    plutovg_canvas_t* canvas;
    pdf_file_t* pdf;
    pdf_page_t* page;
    pdf_obj_t* current_obj;
    pdf_graphics_state_t* state;
    cvector_vector_type(pdf_font_cache_t*) fontcache;
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