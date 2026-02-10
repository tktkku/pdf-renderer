#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>

    //#define CVECTOR_LINEAR_GROWTH
#include "../c-vector/cvector.h"
    typedef struct pdf_parser_token pdf_parser_token_t;

    typedef struct pdf_node pdf_node_t;
    typedef struct pdf_deque pdf_deque_t;

    struct pdf_value;
    typedef struct pdf_value pdf_value_t;
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
    typedef struct pdf_font_descriptor pdf_font_descriptor_t;
    typedef struct pdf_stream pdf_stream_t;

    typedef struct pdf_image pdf_image_t;
    
    typedef struct pdf_form pdf_form_t;
    typedef struct pdf_xobject pdf_xobject_t;

    struct pdf_parser;
    typedef struct pdf_parser pdf_parser_t;

    struct pdf_cmap;
    typedef struct pdf_cmap pdf_cmap_t;

    void pdf_value_free(struct pdf_value* value);
    /**
     * buf: start position
     * size: size of the buffer
     * user_data: custom param
     * return: actual size that filled in buf
     */

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
    void pdf_obj_get_colorspace(pdf_obj_t* obj, const char* name, char* value);

    pdf_file_t* pdf_file_read_file(const char* file_name);
    pdf_file_t* pdf_file_read_buffer(const char* data, size_t size);
    void pdf_file_free(pdf_file_t* file);
    pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, int ref);
    int pdf_file_get_pages(pdf_file_t* pdf);
    pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo);
    pdf_cmap_t* pdf_file_get_cmap(pdf_file_t* pdf, char* name);
    void pdf_file_load_font(pdf_file_t* page, const char* name, const char* data, long len);

    pdf_font_t* pdf_font_init(void);
    void pdf_font_free(pdf_font_t* font);
    pdf_font_t* pdf_font_reference(pdf_font_t* font);
    void pdf_cff_parse(pdf_font_descriptor_t* font_descriptor);

    pdf_array_t* pdf_array_init(void);
    void pdf_array_free(pdf_array_t* array);

    pdf_cmap_t* pdf_cmap_init(void);
    pdf_cmap_t* pdf_cmap_find(const char* name);
    void pdf_cmap_free(pdf_cmap_t* cmap);

    pdf_deque_t* pdf_deque_init();
    void pdf_deque_free(pdf_deque_t* deque);
    void pdf_deque_push(pdf_deque_t* q, const void* data, size_t size);
    void pdf_deque_pop_front(pdf_deque_t* q, pdf_node_t* data);
    void pdf_deque_pop_end(pdf_deque_t* q, pdf_node_t* data);
    void pdf_deque_empty(pdf_deque_t* deque);
    void pdf_deque_get(pdf_deque_t* q, pdf_node_t* data, int index);


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

#ifdef __cplusplus
}
#endif