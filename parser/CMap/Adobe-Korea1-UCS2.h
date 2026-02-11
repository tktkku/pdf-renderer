#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_ADOBE_KOREA1_UCS2[] = {
    {2, 0x0000, 0xffff},
};

pdf_cmap_t cmap_ADOBE_KOREA1_UCS2 = {
    "Adobe-Korea1-UCS2",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    0,   //char_range_map_len
    NULL,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    1,    //code_range_map_len
    code_range_map_ADOBE_KOREA1_UCS2,    //code_range_map
     NULL//next
};

