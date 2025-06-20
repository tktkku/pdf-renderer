#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_ROMAN[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_ROMAN[] = {
    {0x20, 0x7e, 0xe7},
};

pdf_cmap_t cmap_ROMAN = {
    .name = "Roman",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 1,
    .char_range_map = char_range_map_ROMAN,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 1,
    .code_range_map = code_range_map_ROMAN,
    .next = NULL
};

