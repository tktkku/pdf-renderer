#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_ETENMS_B5_H[] = {
    {0x20, 0x7e, 0x1},
};

pdf_cmap_t cmap_ETENMS_B5_H = {
    "ETenms-B5-H",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    1,   //char_range_map_len
    char_range_map_ETENMS_B5_H,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

