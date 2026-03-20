#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>

    class pdf_token;

    class pdf_node;
    class pdf_deque;

    struct pdf_value;
    typedef struct pdf_value pdf_value_t;
    typedef struct pdf_value pdf_obj_value_t;

    struct pdf_obj;
    typedef struct pdf_obj pdf_obj_t;

    struct pdf_stream;
    typedef struct pdf_stream pdf_stream_t;

    class pdf_dict;
    class pdf_array;

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


    int pdf_page_get_streams(pdf_page_t* page);
    pdf_stream_t* pdf_page_get_stream(pdf_page_t* page, int index);

    void pdf_stream_close(pdf_stream_t* stream);
    int pdf_stream_get_data(pdf_stream_t* stream, unsigned char* buf, int size);
    pdf_stream_t* pdf_stream_init(pdf_file_t* pdf, pdf_obj_t* obj, int len, int offset);
    void pdf_stream_free(pdf_stream_t* stream);
    void pdf_stream_open(pdf_stream_t* stream);
    void pdf_stream_get_all(pdf_stream_t* stream, unsigned char** buffer, int* size);


    pdf_file_t* pdf_file_read_file(const char* file_name);
    pdf_file_t* pdf_file_read_buffer(const char* data, size_t size);
    void pdf_file_free(pdf_file_t* file);
    pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, int ref);
    int pdf_file_get_pages(pdf_file_t* pdf);
    pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo);
    void pdf_file_load_font(pdf_file_t* page, const char* name, const char* data, long len);

constexpr double DPI = 203;
constexpr double PIXELS_PER_POINT = (DPI / 72.0); // 1point=1/72inch->72point/1inch dots/inch / point/inch
constexpr double POINTS_PER_PIXEL = (72.0 / DPI); // point/inch / pixels/inch
constexpr double MM_PER_PIXEL = (25.4 / DPI);     // 1inch=25.4mm  mm/inch / dots/inch
constexpr double PIXELS_PER_MM = (DPI / 25.4);    // dots/inch / mm/inch

#ifdef __cplusplus
}
#endif