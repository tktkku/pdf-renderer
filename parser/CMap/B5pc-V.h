#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_B5PC_V[] = {
    {0xa14b, 0xa14b, 0x354e},
    {0xa15a, 0xa15a, 0x35af},
    {0xa15c, 0xa15c, 0x35b1},
    {0xa15d, 0xa15e, 0x82},
    {0xa161, 0xa162, 0x86},
    {0xa165, 0xa166, 0x8a},
    {0xa169, 0xa16a, 0x8e},
    {0xa16d, 0xa16e, 0x92},
    {0xa171, 0xa172, 0x96},
    {0xa175, 0xa176, 0x9a},
    {0xa179, 0xa17a, 0x9e},
    {0xa1e3, 0xa1e3, 0x354f},
};

pdf_cmap_t cmap_B5PC_V = {
    "B5pc-V",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    12,   //char_range_map_len
    char_range_map_B5PC_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

