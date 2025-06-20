#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_KATAKANA[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_KATAKANA[] = {
    {0x20, 0x5f, 0x146},
    {0x60, 0x7e, 0x187},
};

pdf_cmap_t cmap_KATAKANA = {
    .name = "Katakana",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 2,
    .char_range_map = char_range_map_KATAKANA,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 1,
    .code_range_map = code_range_map_KATAKANA,
    .next = NULL
};

