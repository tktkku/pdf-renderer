#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIGB_UTF16_V[] = {
    {0x256, 0x2014},
    {0x257, 0x2026},
    {0x23f, 0x3001},
    {0x23e, 0x3002},
    {0x1e1a, 0x3013},
    {0x242, 0xff01},
    {0x23d, 0xff0c},
    {0x1e1b, 0xff0e},
    {0x1e1c, 0xff1d},
    {0x243, 0xff1f},
    {0x1e1d, 0xff3b},
    {0x1e1e, 0xff3d},
    {0x258, 0xff3f},
    {0x254, 0xff5b},
    {0x255, 0xff5d},
    {0x1e18, 0xff5e},
    {0x1e1f, 0xffe3},
};

static pdf_char_range_map_t char_range_map_UNIGB_UTF16_V[] = {
    {0x3008, 0x300f, 0x248},
    {0x3010, 0x3011, 0x252},
    {0x3014, 0x3015, 0x246},
    {0x3016, 0x3017, 0x250},
    {0xff08, 0xff09, 0x244},
    {0xff1a, 0xff1b, 0x240},
};

pdf_cmap_t cmap_UNIGB_UTF16_V = {
    "UniGB-UTF16-V",    //name
    true,        //worldwide
    17,      //unicode_map_len
    unicode_map_UNIGB_UTF16_V,      //unicode_map
    6,   //char_range_map_len
    char_range_map_UNIGB_UTF16_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

