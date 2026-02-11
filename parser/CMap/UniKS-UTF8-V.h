#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIKS_UTF8_V[] = {
    {0x1f7d, 0xe28096},
    {0x1f7a, 0xe280a5},
    {0x1f8b, 0xe38093},
    {0x1f8c, 0xefbc81},
    {0x1f8f, 0xefbc8c},
    {0x1f90, 0xefbc8e},
    {0x1f97, 0xefbcbb},
    {0x1f98, 0xefbcbd},
    {0x1f99, 0xefbcbf},
    {0x1f7e, 0xefbd9e},
    {0x1f9d, 0xefbfa3},
};

static pdf_char_range_map_t char_range_map_UNIKS_UTF8_V[] = {
    {0xe28093, 0xe28094, 0x1f7b},
    {0xe38081, 0xe38082, 0x1f78},
    {0xe38088, 0xe38091, 0x1f81},
    {0xe38094, 0xe38095, 0x1f7f},
    {0xefbc88, 0xefbc89, 0x1f8d},
    {0xefbc9a, 0xefbc9f, 0x1f91},
    {0xefbd9b, 0xefbd9d, 0x1f9a},
};

pdf_cmap_t cmap_UNIKS_UTF8_V = {
    "UniKS-UTF8-V",    //name
    true,        //worldwide
    11,      //unicode_map_len
    unicode_map_UNIKS_UTF8_V,      //unicode_map
    7,   //char_range_map_len
    char_range_map_UNIKS_UTF8_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

