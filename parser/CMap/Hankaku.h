#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_HANKAKU[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_HANKAKU[] = {
    {0x20, 0x5f, 0xe7},
    {0x60, 0x60, 0xe7},
    {0x61, 0x7e, 0x128},
    {0x81, 0x85, 0x147},
    {0x86, 0x8f, 0x204},
    {0x90, 0x90, 0x156},
    {0x91, 0x9f, 0x20e},
    {0xa1, 0xdf, 0x147},
    {0xe0, 0xfd, 0x21d},
    {0xfe, 0xff, 0x184},
};

pdf_cmap_t cmap_HANKAKU = {
    "Hankaku",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    10,   //char_range_map_len
    char_range_map_HANKAKU,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    1,    //code_range_map_len
    code_range_map_HANKAKU,    //code_range_map
     NULL//next
};

