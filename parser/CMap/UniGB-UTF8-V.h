#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIGB_UTF8_V[] = {
    {0x256, 0xe28094},
    {0x257, 0xe280a6},
    {0x23f, 0xe38081},
    {0x23e, 0xe38082},
    {0x1e1a, 0xe38093},
    {0x242, 0xefbc81},
    {0x23d, 0xefbc8c},
    {0x1e1b, 0xefbc8e},
    {0x1e1c, 0xefbc9d},
    {0x243, 0xefbc9f},
    {0x1e1d, 0xefbcbb},
    {0x1e1e, 0xefbcbd},
    {0x258, 0xefbcbf},
    {0x254, 0xefbd9b},
    {0x255, 0xefbd9d},
    {0x1e18, 0xefbd9e},
    {0x1e1f, 0xefbfa3},
};

static pdf_char_range_map_t char_range_map_UNIGB_UTF8_V[] = {
    {0xe38088, 0xe3808f, 0x248},
    {0xe38090, 0xe38091, 0x252},
    {0xe38094, 0xe38095, 0x246},
    {0xe38096, 0xe38097, 0x250},
    {0xefbc88, 0xefbc89, 0x244},
    {0xefbc9a, 0xefbc9b, 0x240},
};

pdf_cmap_t cmap_UNIGB_UTF8_V = {
    "UniGB-UTF8-V",    //name
    true,        //worldwide
    17,      //unicode_map_len
    unicode_map_UNIGB_UTF8_V,      //unicode_map
    6,   //char_range_map_len
    char_range_map_UNIGB_UTF8_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

