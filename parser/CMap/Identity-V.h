#include "../pdf-private.h"

pdf_cmap_t cmap_IDENTITY_V = {
    "Identity-V",    //name
    true,        //worldwide
    0,      //unicode_map_len
    NULL,      //unicode_map
    0,   //char_range_map_len
    NULL,   //char_range_map
    0,       //not_def_range_len
    NULL,       //not_def_range
    0,    //code_range_map_len
    NULL,    //code_range_map
     NULL//next
};

