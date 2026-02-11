#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNICNS_UTF16_V[] = {
    {0x78, 0x2013},
    {0x7a, 0x2014},
    {0x6d, 0x2025},
    {0x35b1, 0xfe4f},
    {0x86, 0xff5b},
    {0x87, 0xff5d},
};

static pdf_char_range_map_t char_range_map_UNICNS_UTF16_V[] = {
    {0x3008, 0x3009, 0x96},
    {0x300a, 0x300b, 0x92},
    {0x300c, 0x300d, 0x9a},
    {0x300e, 0x300f, 0x9e},
    {0x3010, 0x3011, 0x8e},
    {0x3014, 0x3015, 0x8a},
    {0xff08, 0xff09, 0x82},
};

pdf_cmap_t cmap_UNICNS_UTF16_V = {
    "UniCNS-UTF16-V",    //name
    true,        //worldwide
    6,      //unicode_map_len
    unicode_map_UNICNS_UTF16_V,      //unicode_map
    7,   //char_range_map_len
    char_range_map_UNICNS_UTF16_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

