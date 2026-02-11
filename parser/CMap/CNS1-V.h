#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_CNS1_V[] = {
    {0x212c, 0x212c, 0x354e},
    {0x213b, 0x213b, 0x7c},
    {0x213d, 0x213d, 0x7e},
    {0x213e, 0x213f, 0x82},
    {0x2142, 0x2143, 0x86},
    {0x2146, 0x2147, 0x8a},
    {0x214a, 0x214b, 0x8e},
    {0x214e, 0x214f, 0x92},
    {0x2152, 0x2153, 0x96},
    {0x2156, 0x2157, 0x9a},
    {0x215a, 0x215b, 0x9e},
    {0x2244, 0x2244, 0x354f},
};

pdf_cmap_t cmap_CNS1_V = {
    "CNS1-V",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    12,   //char_range_map_len
    char_range_map_CNS1_V,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

