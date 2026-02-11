#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_ADOBE_KR_0[] = {
    {2, 0x0000, 0x0bff},
};

static pdf_char_range_map_t char_range_map_ADOBE_KR_0[] = {
    {0x0000, 0x00ff, 0x0},
    {0x0100, 0x01ff, 0x100},
    {0x0200, 0x02ff, 0x200},
    {0x0300, 0x03ff, 0x300},
    {0x0400, 0x04ff, 0x400},
    {0x0500, 0x05ff, 0x500},
    {0x0600, 0x06ff, 0x600},
    {0x0700, 0x07ff, 0x700},
    {0x0800, 0x08ff, 0x800},
    {0x0900, 0x09ff, 0x900},
    {0x0a00, 0x0aff, 0xa00},
    {0x0b00, 0x0bf2, 0xb00},
};

pdf_cmap_t cmap_ADOBE_KR_0 = {
    "Adobe-KR-0",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    12,   //char_range_map_len
    char_range_map_ADOBE_KR_0,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    1,    //code_range_map_len
    code_range_map_ADOBE_KR_0,    //code_range_map
     NULL//next
};

