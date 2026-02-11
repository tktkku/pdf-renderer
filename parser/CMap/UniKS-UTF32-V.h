#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIKS_UTF32_V[] = {
    {0x1f7d, 0x00002016},
    {0x1f7a, 0x00002025},
    {0x1f8b, 0x00003013},
    {0x1f8c, 0x0000ff01},
    {0x1f8f, 0x0000ff0c},
    {0x1f90, 0x0000ff0e},
    {0x1f97, 0x0000ff3b},
    {0x1f98, 0x0000ff3d},
    {0x1f99, 0x0000ff3f},
    {0x1f7e, 0x0000ff5e},
    {0x1f9d, 0x0000ffe3},
};

static pdf_char_range_map_t char_range_map_UNIKS_UTF32_V[] = {
    {0x00002013, 0x00002014, 0x1f7b},
    {0x00003001, 0x00003002, 0x1f78},
    {0x00003008, 0x00003011, 0x1f81},
    {0x00003014, 0x00003015, 0x1f7f},
    {0x0000ff08, 0x0000ff09, 0x1f8d},
    {0x0000ff1a, 0x0000ff1f, 0x1f91},
    {0x0000ff5b, 0x0000ff5d, 0x1f9a},
};

pdf_cmap_t cmap_UNIKS_UTF32_V = {
    "UniKS-UTF32-V",    //name
    true,        //worldwide
    11,      //unicode_map_len
    unicode_map_UNIKS_UTF32_V,      //unicode_map
    7,   //char_range_map_len
    char_range_map_UNIKS_UTF32_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

