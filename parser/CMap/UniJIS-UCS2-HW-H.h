#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_UNIJIS_UCS2_HW_H[] = {
    {0x0020, 0x005b, 0xe7},
    {0x005c, 0x005c, 0x220f},
    {0x005d, 0x007e, 0x124},
    {0x00a5, 0x00a5, 0x123},
};

pdf_cmap_t cmap_UNIJIS_UCS2_HW_H = {
    "UniJIS-UCS2-HW-H",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    4,   //char_range_map_len
    char_range_map_UNIJIS_UCS2_HW_H,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

