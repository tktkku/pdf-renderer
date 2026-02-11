#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_ROMAN[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_ROMAN[] = {
    {0x20, 0x7e, 0xe7},
};

pdf_cmap_t cmap_ROMAN = {
    "Roman",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    1,   //char_range_map_len
    char_range_map_ROMAN,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    1,    //code_range_map_len
    code_range_map_ROMAN,    //code_range_map
     NULL//next
};

