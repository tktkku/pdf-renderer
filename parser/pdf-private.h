#ifndef _PDF_COMMON_H_
#define _PDF_COMMON_H_
#include "pdf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <zlib.h>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
typedef enum
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
} PdfTokenType;

class PdfToken
{
private:
    char* token;
    PdfTokenType type;
    int token_len;
    int steps;
    PdfToken* next;
    friend class PdfParser;
    ~PdfToken();
public:
    PdfToken() = default;
    const char* getValue() const;
    int getLen() const;
    PdfTokenType getType() const;
};

enum PdfValueType
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

struct PdfValue
{
    union {
        bool boolean;
        char* name;
        char* string;
        int indirect;
        double number;
        PdfDict* dict;
        PdfArray* array;
    };
    int value_len;
    enum PdfValueType type;
    PdfValue()
        : type(NUL), indirect(0)
    {
    }
    ~PdfValue();
};


class PdfObj
{
public:
    int seq;
    PdfValue* value;
    PdfStream* stream;
    pdf_xobject_t* xobject;
    unsigned char* font_data;
    int font_data_len;
    pdf_file_t* pdf;
public:
    PdfObj()
        : seq(0), value(nullptr), stream(nullptr), xobject(nullptr),
        font_data(nullptr), font_data_len(0), pdf(nullptr)
    {
    }
    pdf_xobject_t* getXobject();
    ~PdfObj();
};

class PdfArray
{
private:
    std::vector<PdfValue*> elements;
public:
    PdfArray(const PdfArray&) = delete;
    PdfArray& operator=(const PdfArray&) = delete;
    PdfArray() {}
    ~PdfArray();
    size_t size() const
    {
        return elements.size();
    }
    void push(PdfValue* v)
    {
        elements.emplace_back(v);
    }
    PdfValue* operator[](int index) const;
};
class PdfDict
{
private:
    std::map<std::string, PdfValue*> entries;
public:
    PdfDict() {}
    PdfDict(const PdfDict&) = delete;
    PdfDict& operator=(const PdfDict&) = delete;
    ~PdfDict();
    size_t size() const
    {
        return entries.size();
    }
    PdfValue& operator[](const std::string& key);
    // const pdf_value* operator[](const std::string& key) const
    // {
    //     auto it = entries.find(key);
    //     if (it != entries.end())
    //         return it->second;
    //     else
    //         return nullptr;
    // }
};
// struct pdf_dict_pair
// {
//     char* name;
//     int name_len;
//     pdf_dict_pair_value_t* value;
// };

// struct pdf_dict
// {
//     int num_pairs;
//     pdf_dict_pair_t** pairs;
// };

// struct pdf_array
// {
//     int num_elements;
//     pdf_array_element_value_t** values;
// };

struct PdfRect
{
    double x, y, width, height;
};

typedef enum xref_type
{
    UNCOMPRESSED = 0,
    COMPRESSED
} xref_type_t;

typedef struct
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

// typedef struct xref_table {
//     int size;
//     xref_t* xrefs;
// } xref_table_t;

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
    int unicode_map_len;
    pdf_unicode_map_t* unicode_map;
    int char_range_map_len;
    pdf_char_range_map_t* char_range_map;
    int code_range_map_len;
    pdf_code_range_map_t* code_range_map;
    struct pdf_cmap* next;
};

struct pdf_file
{
    FILE* pFile;
    long data_len;
    // int num_read_objs;
    // pdf_obj_t** read_objs;
    std::vector<PdfObj*> read_objs;
    // xref_table_t* xref_table;
    std::vector<xref_t*> xref_table;
    // pdf_obj_t** pages;
    // int num_pages;
    std::vector<PdfObj*> pages;
    std::vector<pdf_cmap_t*> cmaps;
};

struct pdf_resources
{
    PdfDict* ext_gstate;
    PdfDict* color_space;
    PdfDict* pattern;
    PdfDict* shading;
    PdfDict* xobject_dict;
    PdfDict* font_dict;
    PdfArray* proc_set;
    PdfDict* properties;
};

class PdfPage
{
public:
    int pageNo;
    pdf_file_t* pdf;
    PdfRect crop_box;
    PdfRect media_box;
    // pdf_obj_t** contents;
    // int num_contents;
    std::vector<PdfObj*> contents;
    int rotate;
    pdf_resources_t* resources;
    PdfArray* annots;
public:
    PdfPage()
        : pageNo(0), pdf(nullptr), rotate(0), resources(nullptr), annots(nullptr)
    {
    }
    ~PdfPage();
    int getMediaWidth() { return media_box.width; }
    int getMediaHeight() { return media_box.height; }
    pdf_font_t* getFont(const char* name);

    void getExtGState(const char* name);
    int getStreams();
    PdfStream* getStream(int index);
private:
    pdf_font_t* _loadType0Font(PdfDict& font_dict);
    pdf_font_t* _loadTruetypeFont(PdfDict& font_dict);
};


struct pdf_font
{
    char* type;
    char* subtype;
    char* basefont;
    char* encoding;
    pdf_cmap_t* to_unicode_map;
    PdfDict* descendant_font_dict;
    PdfDict* font_descriptor;
    int font_weight;
    int flags;
    int italic_angle;
    PdfArray* font_bbox;
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
    PdfArray* w_aar;
    char* cid_to_gid_map;
    int cid_to_gid_map_ref;
    unsigned char* font_data;
    int font_data_length;
    pdf_cmap_t* cmap;
    int first_char;
    int last_char;
    PdfArray* widths;
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
};
typedef enum {
    BUFFER_READER,
    FILE_READER,
    STREAM_READER
} PdfParserReadType;

class PdfParser
{
private:
    using ParserReadFunc = void (PdfParser::*)(void*);
    struct {
        PdfParserReadType type;
        void* source;
        ParserReadFunc read;
    } reader;
    pdf_file_t* pdf;
    unsigned char buffer[4096];
    int buffer_size;
    unsigned char* end_pos;
    unsigned char* splite_pos;
    unsigned char* current_pos;
    struct
    {
        unsigned char* rem;
        int len;
    } remain;
    std::vector<PdfToken*> token_cache;
    PdfToken* freedTokens;
public:
    /**
     * @param pdf
     * @param type BUFFER_READER, FILE_READER, STREAM_READER
     * @param source pdf_buffer_t*, FILE*, pdf_stream_t*
     */
    PdfParser(pdf_file_t* pdf, PdfParserReadType type, void* source);
    ~PdfParser();

    PdfToken* getNextToken();
    void freeToken(PdfToken* token);
    PdfObj* buildObj();
    PdfDict* buildDict();
    PdfArray* buildArray();
    pdf_cmap_t* buildCMap();
private:
    PdfToken* _buildNumber(const unsigned char* start, const unsigned char* end);
    PdfToken* _buildToken(const unsigned char* start, PdfTokenType type, int len);
    PdfToken* _getNextToken();
    PdfToken* _getNextOneToken(const unsigned char* start, const unsigned char* end);
    int _copyRem();
    void _split(int end_i);
    void _readFile(void* source);
    void _readBuffer(void* source);
    void _readStream(void* source);
    void _setCommonValue(PdfToken* tk, PdfValue& p);
};

struct pdf_buffer
{
    unsigned char* buffer;
    int buffer_size;
    int processed;
};

class PdfStream
{
private:
    pdf_file_t* pdf;
    PdfObj* obj;


    PdfParser* parser;
    PdfValue* filter;
    int predictor;
    int colors;
    int bitspercomponent;
    int columns;
    int earlychange;
public:
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
public:
    PdfStream(pdf_file_t* pdf, PdfObj* obj, int len, int offset);
    ~PdfStream();
    void close();
    void open();

    PdfToken* getNextToken();
    void freeToken(PdfToken* token) { parser->freeToken(token); }
    int getData(unsigned char* buf, int size);
    void getAll(unsigned char** buffer, int* size);

};

// struct pdf_stack_node
// {
//     char* data;
//     size_t size;
//     struct pdf_stack_node* next;
// };

// struct pdf_stack
// {
//     pdf_stack_node_t* top;
// };
bool _is_space(char c);
bool _is_hex(char c);
bool _is_digit(char c);
bool _is_delimiter(char c);
uint16_t _hex_str_to_16bit(char hexStr[4]);
uint8_t _hex_str_to_8bit(char hexStr[2]);


#endif