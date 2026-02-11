#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_KATAKANA[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_KATAKANA[] = {
    {0x20, 0x5f, 0x146},
    {0x60, 0x7e, 0x187},
};

pdf_cmap_t cmap_KATAKANA = {
    "Katakana",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    2,   //char_range_map_len
    char_range_map_KATAKANA,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    1,    //code_range_map_len
    code_range_map_KATAKANA,    //code_range_map
     NULL//next
};

