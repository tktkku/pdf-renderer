#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIMANGA_UTF16_V[] = {
    {0x3cb3, 0xfe33},
};

static pdf_char_range_map_t char_range_map_UNIMANGA_UTF16_V[] = {
    {0xfe10, 0xfe19, 0x3ca6},
    {0xfe30, 0xfe31, 0x3cb0},
    {0xfe35, 0xfe44, 0x3cb5},
    {0xfe47, 0xfe48, 0x3cc7},
};

pdf_cmap_t cmap_UNIMANGA_UTF16_V = {
    "UniManga-UTF16-V",    //name
    true,        //worldwide
    1,      //unicode_map_len
    unicode_map_UNIMANGA_UTF16_V,      //unicode_map
    4,   //char_range_map_len
    char_range_map_UNIMANGA_UTF16_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

