#ifndef _PDF_COMMON_H_
#define _PDF_COMMON_H_
#include "pdf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "zlib.h"
//#define CVECTOR_LINEAR_GROWTH
#include "cvector.h"
#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
enum pdf_parser_token_type
{
    TOKEN_NULL = 0,
    TOKEN_BOOLEAN_TRUE,
    TOKEN_BOOLEAN_FALSE,
    TOKEN_SPACE,
    TOKEN_NEWLINE,
    TOKEN_COMMENT,
    TOKEN_OPERATOR,
    TOKEN_INDIRECT,
    TOKEN_OBJ_BEG,
    TOKEN_OBJ_END,
    TOKEN_STREAM_BEG,
    TOKEN_STREAM_END,
    TOKEN_NAME,
    TOKEN_DICT_BEG,
    TOKEN_DICT_END,
    TOKEN_ARRAY_BEG,
    TOKEN_ARRAY_END,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_HEX_STRING,
    TOKEN_XREF,
    TOKEN_FINDRESOURCE,
    TOKEN_BEGIN,
    TOKEN_DICT,
    TOKEN_BEGINCMAP,
    TOKEN_DEF,
    TOKEN_DUP,
    TOKEN_END,
    TOKEN_BEGINCODESPACERANGE,
    TOKEN_ENDCODESPACERANGE,
    TOKEN_BEGINBFCHAR,
    TOKEN_ENDBFCHAR,
    TOKEN_ENDCMAP,
    TOKEN_BEGINCIDCHAR,
    TOKEN_ENDCIDCHAR,
    TOKEN_BEGINCIDRANGE,
    TOKEN_ENDCIDRANGE,
    TOKEN_BEGINBFRANGE,
    TOKEN_ENDBFRANGE,
};

struct pdf_parser_token
{
    pdf_parser_token_type_t type;
    char* token;
    int token_len;
    int steps;
    struct pdf_parser_token* next;
};

enum pdf_value_type
{
    NUL,
    BOOLEAN,
    NAME,
    INDIRECT,
    NUMBER,
    ARRAY,
    DICT,
    STRING
};

struct pdf_value
{
    union {
        bool boolean;
        char* name;
        char* string;
        int indirect;
        double number;
        pdf_dict_t* dict;
        pdf_array_t* array;
    } val;
    int value_len;
    enum pdf_value_type type;
};


struct pdf_obj
{
    int seq;
    pdf_obj_value_t* value;
    pdf_stream_t* stream;
    pdf_xobject_t* xobject;
    unsigned char* font_data;
    int font_data_len;
    pdf_file_t* pdf;
};

struct pdf_dict_pair
{
    char* name;
    int name_len;
    pdf_dict_pair_value_t* value;
};

struct pdf_dict
{
    cvector_vector_type(pdf_dict_pair_t*) pairs;
};

struct pdf_array
{
    int num_elements;
    pdf_array_element_value_t** values;
};

typedef struct rect_d
{
    double x, y, width, height;
} rect_d_t;

typedef enum xref_type
{
    UNCOMPRESSED = 0,
    COMPRESSED
} xref_type_t;

typedef struct xref
{
    xref_type_t type;
    int sequence;
    union
    {
        struct
        {
            long offset;
        } uncompressed;

        struct
        {
            int ref;
            int index;
        } compressed;
    };
    int generation;
    char inuse;
} xref_t;

typedef struct xref_table {
    int size;
    xref_t* xrefs;
} xref_table_t;

typedef struct
{
    uint16_t cid;
    uint16_t unicode;
} pdf_unicode_map_t;

typedef struct
{
    uint16_t srcStart;
    uint16_t srcEnd;
    uint16_t dstStart;
} pdf_char_range_map_t;

typedef struct
{
    uint16_t srcStart;
    uint16_t srcEnd;
} pdf_code_range_map_t;

struct pdf_cmap
{
    char name[256];
    bool worldwide;
    // int unicode_map_len;
    // pdf_unicode_map_t* unicode_map;
    cvector_vector_type(pdf_unicode_map_t*) unicode_map;
    // int char_range_map_len;
    // pdf_char_range_map_t* char_range_map;
    cvector_vector_type(pdf_char_range_map_t*) char_range_map;
    // int code_range_map_len;
    // pdf_code_range_map_t* code_range_map;
    cvector_vector_type(pdf_code_range_map_t*) code_range_map;
    struct pdf_cmap* next;
};

struct pdf_file
{
    FILE* pFile;
    long data_len;
    long current_index;
    //int num_read_objs;
    // pdf_obj_t** read_objs;
    cvector_vector_type(pdf_obj_t*) read_objs;
    int root_obj_ref;
    int info_obj_ref;
    // xref_table_t* xref_table;
    cvector_vector_type(xref_t*) xref_table;
    pdf_obj_t** pages;
    int num_pages;

    pdf_cmap_t* cmaps;
    int num_cmaps;
    pdf_parser_token_t* freed_tokens;
};

struct pdf_resources
{
    pdf_dict_t* ext_gstate;
    pdf_dict_t* color_space;
    pdf_dict_t* pattern;
    pdf_dict_t* shading;
    pdf_dict_t* xobject_dict;
    pdf_dict_t* font_dict;
    pdf_array_t* proc_set;
    pdf_dict_t* properties;
};

struct pdf_page
{
    int pageNo;
    pdf_file_t* pdf;
    rect_d_t crop_box;
    rect_d_t media_box;
    pdf_obj_t** contents;
    int num_contents;
    int cur_content_index;
    int rotate;
    pdf_resources_t* resources;
    pdf_array_t* annots;
};


struct pdf_font
{
    char* type;
    char* subtype;
    char* basefont;
    char* encoding;
    pdf_cmap_t* to_unicode_map;
    pdf_dict_t* descendant_font_dict;
    pdf_dict_t* font_descriptor;
    int font_weight;
    int flags;
    int italic_angle;
    pdf_array_t* font_bbox;
    int ascent;
    int descent;
    int cap_height;
    int stemv;
    int cid_set_ref;
    int fontfile1_ref;
    int fontfile2_ref;
    int fontfile3_ref;
    int cid_system_info_ref;
    int dw;
    pdf_array_t* w_aar;
    unsigned char* cid_to_gid_map;
    int cid_to_gid_map_ref;
    unsigned char* font_data;
    int font_data_length;
    pdf_cmap_t* cmap;
    bool load_succeed;
    int first_char;
    int last_char;
    pdf_array_t* widths;
};

struct pdf_image
{
    int width;
    int height;
    int bits_per_color;
    unsigned char* data;
    int data_len;
    char color_space[128];
};

enum xobject_type
{
    XOBJ_IMAGE = 0,
    XOBJ_FORM
};

struct pdf_form
{
    double matrix[6];
    double bbox[4];
};

struct pdf_xobject
{
    xobject_type_t type;
    union
    {
        pdf_form_t* form;
        pdf_image_t* image;
    };
};
enum pdf_parser_reader_type {
    BUFFER_READER,
    FILE_READER,
    STREAM_READER
};

struct pdf_parser
{
    struct {
        enum pdf_parser_reader_type type;
        void* source;
        pdf_parser_read_func read;
    } reader;
    pdf_file_t* pdf;
    unsigned char* buffer;
    int buffer_size;
    unsigned char* end_pos;
    unsigned char* splite_pos;
    unsigned char* current_pos;
    struct
    {
        unsigned char* rem;
        int len;
    } remain;
    pdf_parser_token_t* cached_tokens[3];
    int num_cached_tokens;
    bool pause_read;
};

struct pdf_buffer
{
    unsigned char* buffer;
    int buffer_size;
    int processed;
};

struct pdf_stream
{
    pdf_file_t* pdf;
    pdf_obj_t* obj;
    int stream_offset;
    int stream_len;
    struct
    {
        z_stream flate;
        // temp buffer
        unsigned char* buf;
        // temp buffer size
        int buf_size;
        // read in data len
        int len;
        int cur_pos;
    } decomp;
    int processed;
    int readin_len;
    pdf_parser_t* parser;
    struct pdf_value* filter;
    int predictor;
    int colors;
    int bitspercomponent;
    int columns;
    int earlychange;
};

struct pdf_stack_node
{
    char* data;
    size_t size;
    struct pdf_stack_node* next;
};

struct pdf_stack
{
    pdf_stack_node_t* top;
};
bool _is_space(char c);
bool _is_hex(char c);
bool _is_digit(char c);
bool _is_delimiter(char c);
void _pdf_parser_read_file(pdf_parser_t* parser, void* source);
void _pdf_parser_read_buffer(pdf_parser_t* parser, void* source);
void _pdf_parser_read_stream(pdf_parser_t* parser, void* source);
uint16_t _hex_str_to_16bit(char hexStr[4]);
uint8_t _hex_str_to_8bit(char hexStr[2]);

#endif