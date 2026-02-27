#pragma once
#include "pdf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "zlib.h"
#include <string>
#include <vector>
#include <map>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef enum pdf_token_type
{
#define TOKEN_DEF(v, t) t,
#include "pdf-token.def"
} pdf_token_type_t;

class pdf_token
{
private:
    pdf_token_type_t _type;
    std::vector<char> _data;
    int _steps;
public:
    explicit pdf_token(pdf_token_type_t type)
    {
        _type = type;
        _data.push_back('\0');
    }
    explicit pdf_token(const char* start, int len, pdf_token_type_t type)
    {
        _type = type;
        _steps = len;
        if (start != NULL)
        {
            _data.insert(_data.end(), start, start + len);
        }
        
        if (type == TOKEN_NAME)
        {
            decode_name();
        }
        else if (type == TOKEN_STRING)
        {
            decode_string();
        }
        else if (type == TOKEN_HEX_STRING)
        {
            decode_hex_string();
        }
        _data.push_back('\0');
    }
    explicit pdf_token(const char* start, int len, pdf_token_type_t type, int steps)
    {
        _type = type;
        _steps = steps;
        if (start != NULL)
        {
            _data.insert(_data.end(), start, start + len);
        }
        
        if (type == TOKEN_NAME)
        {
            decode_name();
        }
        else if (type == TOKEN_STRING)
        {
            decode_string();
        }
        else if (type == TOKEN_HEX_STRING)
        {
            decode_hex_string();
        }
        _data.push_back('\0');
    }
    void append(pdf_token* other)
    {
        _data.pop_back();
        _data.insert(_data.end(), other->_data.begin(), other->_data.end() - 1);
        _data.push_back('\0');
        _steps += other->steps();
    }
    void append(const char* data, int len)
    {
        _data.pop_back();
        _data.insert(_data.end(), data, data + len);
        _data.push_back('\0');
    }
    const char* data()
    {
        return _data.data();
    }

    pdf_token_type_t type()
    {
        return _type;
    }
    size_t size()
    {
        return _data.size() - 1;
    }
    size_t steps()
    {
        return _steps;
    }
    bool empty()
    {
        return size() == 0;
    }
private:
    void decode_hex_string();
    void decode_name();
    void decode_string();

};

typedef enum pdf_value_type
{
    PDF_VALUE_NULL,
    PDF_VALUE_BOOLEAN,
    PDF_VALUE_NAME,
    PDF_VALUE_INDIRECT,
    PDF_VALUE_NUMBER,
    PDF_VALUE_ARRAY,
    PDF_VALUE_DICT,
    PDF_VALUE_STRING
} pdf_value_type_t;

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
    pdf_value_type_t type;
};


struct pdf_obj
{
    int seq;
    pdf_obj_value_t* value;
    pdf_stream_t* stream;
    struct
    {
        pdf_dict_t* extgstate_dict;
        pdf_dict_t* colorspace_dict;
        pdf_dict_t* pattern_dict;
        pdf_dict_t* shading_dict;
        pdf_dict_t* xobject_dict;
        pdf_dict_t* font_dict;
        pdf_array_t* procset_arr;
        pdf_dict_t* properties_dict;
    } resources;
    pdf_xobject_t* xobject;
    pdf_font_t* font;
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
    std::vector<pdf_dict_pair_t*> pairs;
};

class pdf_array
{
private:
    std::vector<pdf_value_t*> elements;
public:
    size_t size()
    {
        return elements.size();
    }
    void add(pdf_value_t* value)
    {
        elements.push_back(value);
    }
    pdf_value_t* get(size_t index)
    {
        if (index >= elements.size())
        {
            return NULL;
        }
        return elements[index];
    }
    pdf_value_t* operator[](size_t index)
    {
        return get(index);
    }
    pdf_array() {};
    ~pdf_array()
    {
        for (size_t i = 0; i < elements.size(); i++)
        {
            pdf_value_free(elements[i]);
            elements[i] = NULL;
        }
        elements.clear();
    };
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

struct pdf_cmap_unicode_map
{
    uint32_t code;
    uint32_t unicode;
};

struct pdf_cmap_cid_map
{
    uint32_t code;
    uint32_t cid;
};

struct pdf_cmap_char_range
{
    uint32_t srcStart;
    uint32_t srcEnd;
    uint32_t dstStart;
} ;
struct pdf_cmap_code_range
{
    uint8_t byte_len;
    uint32_t srcStart;
    uint32_t srcEnd;
};

struct pdf_cmap
{
    char name[256];
    std::vector<pdf_cmap_cid_map> cid_map;
    std::vector<pdf_cmap_char_range> cid_range_map;
    std::vector<pdf_cmap_unicode_map> unicode_map;
    std::vector<pdf_cmap_char_range> unicode_range_map;

    std::vector<pdf_cmap_char_range> not_def_range;
    std::vector<pdf_cmap_code_range> code_range_map;
    pdf_cmap() {};
    ~pdf_cmap() {};
};
typedef enum {
    INPUT_TYPE_FILE,
    INPUT_TYPE_BUFFER,
    INPUT_TYPE_STREAM
} input_type_t;

typedef struct input
{
    input_type_t type;
    union 
    {
        FILE* file;
        struct
        {
            const char* data;
            size_t size;
            size_t pos;
        } buffer;
        pdf_stream_t* stream;
    };
} input_t;
typedef struct external_font {
    char* name;
    int name_len;
    char* data;
    long data_len;
} pdf_external_font_t;
struct pdf_file
{
    input_t* input;
    long data_len;
    long current_index;
    std::vector<pdf_obj_t*> read_objs;
    int root_obj_ref;
    int info_obj_ref;
    std::vector<xref_t*> xref_table;
    std::vector<pdf_obj_t*> pages;

    std::vector<pdf_cmap_t*> cmaps;
    std::vector<pdf_external_font_t*> external_fonts;
};

// struct pdf_resources
// {
//     pdf_dict_t* ext_gstate;
//     pdf_dict_t* color_space;
//     pdf_dict_t* pattern;
//     pdf_dict_t* shading;
//     pdf_dict_t* xobject_dict;
//     pdf_dict_t* font_dict;
//     pdf_array_t* proc_set;
//     pdf_dict_t* properties;
// };

struct pdf_page
{
    int pageNo;
    pdf_file_t* pdf;
    pdf_obj_t* obj;
    rect_d_t crop_box;
    rect_d_t media_box;
    pdf_obj_t** contents;
    int num_contents;
    int cur_content_index;
    int rotate;
    //pdf_resources_t* resources;
    pdf_array_t* annots;
};

typedef enum
{
    FONT_SUBTYPE_TYPE0 = 0,
    FONT_SUBTYPE_TYPE1,
    FONT_SUBTYPE_TYPE3,
    FONT_SUBTYPE_TRUETYPE,
    FONT_SUBTYPE_CIDFONTTPYE0,
    FONT_SUBTYPE_CIDFONTTPYE2,
} pdf_font_subtype_t;

typedef struct {
    char* registry;
    char* ordering;
    int supplement;
} cid_system_info_t;

struct pdf_font_descriptor
{
    // Type=FontDescriptor
    char* fontName;
    char* fontFamily;
    char* fontStretch;
    double fontWeight;
    uint32_t flags;
    pdf_array_t* fontBBox;
    double italicAngle;
    double ascent;
    double descent;
    double leading;
    double capHeight;
    double xHeight;
    double stemV;
    double stemH;
    double avgWidth;
    double maxWidth;
    double missingWidth;
    unsigned char* fontfile;
    int fontfile_len;
    char* charSet;
    // cff
    pdf_array_t* charstrings;
    pdf_array_t* font_dict_arr;
    pdf_array_t* font_dict_select_arr;
    pdf_array_t* global_subr;
    uint16_t global_subr_bias;
    pdf_array_t* font_matrix;

    //fot cidfonts
    pdf_dict_t* style;
    char* lang;
    pdf_dict_t* fd;
    pdf_dict_t* cidSet;
};

typedef struct
{
    pdf_cmap_t* encoding;
    pdf_font_t *descendant;
    pdf_cmap_t* to_unicode_map;
} pdf_font_type0_t;
typedef struct
{
    char* name;
    int first_char;
    int last_char;
    pdf_array_t* widths;
    pdf_font_descriptor_t* font_descriptor;
    char* encoding;
    pdf_dict_t* encoding_dict;
    pdf_cmap_t* to_unicode_map;
    pdf_array_t* differences;
} pdf_font_type1_t;
typedef struct
{
    char* name;
    char* encoding;
    pdf_dict_t* encoding_dict;
    pdf_array_t* font_matrix;
    pdf_array_t* font_bbox;
    pdf_dict_t* charProcs;
    int first_char;
    int last_char;
    pdf_array_t* widths;
    pdf_font_descriptor_t* font_descriptor;
    pdf_dict_t* resources;
    pdf_cmap_t* to_unicode_map;
    pdf_array_t* differences;
} pdf_font_type3_t;
typedef struct 
{
    cid_system_info_t cid_system_info;
    pdf_cmap_t* cid_to_gid_map; // default: Identity
    int dw; // default: 1000
    pdf_array_t* w_aar;
    pdf_array_t* dw2_aar;
    pdf_array_t* w2_aar;
    pdf_font_descriptor_t* font_descriptor;
} pdf_font_cidfont_t;

struct pdf_font
{
    // Type = /Font
    // Subtype
    pdf_font_subtype_t subtype;
    char* basefont;

    union {
        pdf_font_type0_t* type0;
        pdf_font_type1_t* type1_truetype;
        pdf_font_type3_t* type3;
        pdf_font_cidfont_t* cidfont;
    };
    int references;
};


// struct pdf_font
// {
//     char* type;
//     char* subtype;
//     char* basefont;
//     char* encoding;
//     pdf_dict_t* encoding_dict;
//     pdf_array_t* differences;
//     pdf_dict_t* charProcs;
//     pdf_cmap_t* to_unicode_map;
//     pdf_dict_t* descendant_font_dict;
//     pdf_dict_t* font_descriptor;
//     int font_weight;
//     uint32_t flags;
//     int italic_angle;
//     pdf_array_t* font_matrix;
//     pdf_array_t* font_bbox;
//     int ascent;
//     int descent;
//     int cap_height;
//     int stemv;
//     int cid_set_ref;
//     int fontfile1_ref;
//     int fontfile2_ref;
//     int fontfile3_ref;
//     int cid_system_info_ref;
//     cid_system_info_t cid_system_info;
//     int dw;
//     pdf_array_t* w_aar;
//     unsigned char* cid_to_gid_map;
//     int cid_to_gid_map_ref;
//     unsigned char* font_data;
//     int font_data_length;
//     pdf_cmap_t* cmap;
//     int first_char;
//     int last_char;
//     pdf_array_t* widths;

//     pdf_array_t* charstrings;
//     pdf_array_t* font_dict_arr;
//     /*
//         the first element specifies format
//         if == 0 : 
//             fd = font_dict_select_arr[gid + 1];
//         if == 3 :
//             for i in ranges where i > 0:
//                 if font_dict_select_arr[i] <= gid <= font_dict_select_arr[i + 1]:
//                     fd = font_dict_select_arr[i + 2]
//     */
//     pdf_array_t* font_dict_select_arr;
//     pdf_array_t* global_subr;
//     uint16_t global_subr_bias;
//     int references;
// };

struct pdf_image
{
    int width;
    int height;
    int bits_per_color;
    unsigned char* data;
    int data_len;
    char color_space[128];
};
typedef enum xobject_type
{
    XOBJ_IMAGE = 0,
    XOBJ_FORM
} xobject_type_t;

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
    pdf_obj_t* obj;
};

typedef enum pdf_parser_reader_type {
    INPUT_READER,
    STREAM_READER
}pdf_parser_reader_type_t;
typedef void (*pdf_parser_read_func)(pdf_parser_t* parser, void* source);
struct pdf_parser
{
    pdf_file_t* pdf;
    input_t* input;
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
    pdf_token_t* cached_tokens[3];
    int num_cached_tokens;
    bool pause_read;
    bool eof;
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

bool _is_space(char c);
bool _is_hex(char c);
bool _is_digit(char c);
bool _is_delimiter(char c);
void _pdf_parser_read_input(pdf_parser_t* parser);
uint32_t _str_to_32bit(char* str, int len);
uint32_t _hex_str_to_32bit(const char* hexStr, int len);
uint16_t _hex_str_to_16bit(char hexStr[4]);
uint8_t _hex_str_to_8bit(char hexStr[2]);
const char* _token_to_string(pdf_token_type_t type);
struct pdf_node
{
    void* data;
    size_t size;
    struct pdf_node* prev;
    struct pdf_node* next;
};

struct pdf_deque
{
    pdf_node_t* front;
    pdf_node_t* rear;
    size_t size;
};

int input_file(input_t** input, const char* filename);
int input_buffer(input_t** input, const char* data, size_t size);
size_t input_read(input_t* input, void* ptr, size_t size);
int input_seek(input_t* input, long offset, int whence);
long input_tell(input_t* input);
void input_close(input_t* input);
int input_stream(input_t** input, pdf_stream_t* stream);


/**
 * @param pdf
 * @param type BUFFER_READER, FILE_READER, STREAM_READER
 * @param source pdf_buffer_t*, FILE*, pdf_stream_t*
 */
pdf_parser_t* pdf_parser_init(pdf_file_t* pdf, input_t* input);
void pdf_parser_free(pdf_parser_t* parser);
size_t pdf_parser_read_data(pdf_parser_t* parser, void* ptr, size_t size);
pdf_token_t* pdf_parser_next_token(pdf_parser_t* parser);
pdf_obj_t* pdf_parser_build_obj(pdf_parser_t* parser);
pdf_dict_t* pdf_parser_build_dict(pdf_parser_t* parser);
pdf_array_t* pdf_parser_build_array(pdf_parser_t* parser);
pdf_cmap_t* pdf_parser_build_cmap(pdf_parser_t* parser);

    pdf_dict_t* pdf_dict_init(void);
    void pdf_dict_free(pdf_dict_t* dict);
    double pdf_dict_get_number(pdf_dict_t* dict, const char* name);
    int pdf_dict_get_ref(pdf_dict_t* dict, const char* name);
    pdf_array_t* pdf_dict_get_array(pdf_dict_t* dict, const char* name);
    pdf_dict_t* pdf_dict_get_dict(pdf_dict_t* dict, const char* name);
    char* pdf_dict_get_name(pdf_dict_t* dict, const char* name);
    int pdf_dict_get_bool(pdf_dict_t* dict, const char* name);
    bool pdf_dict_add_array(pdf_dict_t* dict, const char* name, pdf_array_t* array);
    bool pdf_dict_add(pdf_dict_t* dict, const char* name, pdf_value_type_t type, void* data);
    char* pdf_dict_get_string(pdf_dict_t* dict, const char* name);
    bool pdf_dict_add_value(pdf_dict_t* dict, const char* name, pdf_value_t* value);
