#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIMANGA_UTF32_V[] = {
    {0x3cb3, 0x0000fe33},
};

static pdf_char_range_map_t char_range_map_UNIMANGA_UTF32_V[] = {
    {0x0000fe10, 0x0000fe19, 0x3ca6},
    {0x0000fe30, 0x0000fe31, 0x3cb0},
    {0x0000fe35, 0x0000fe44, 0x3cb5},
    {0x0000fe47, 0x0000fe48, 0x3cc7},
};

pdf_cmap_t cmap_UNIMANGA_UTF32_V = {
    "UniManga-UTF32-V",    //name
    true,        //worldwide
    1,      //unicode_map_len
    unicode_map_UNIMANGA_UTF32_V,      //unicode_map
    4,   //char_range_map_len
    char_range_map_UNIMANGA_UTF32_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

