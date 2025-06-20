#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_HIRAGANA[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_HIRAGANA[] = {
    {0x20, 0x20, 0x203},
    {0x21, 0x25, 0x147},
    {0x26, 0x2f, 0x204},
    {0x30, 0x30, 0x156},
    {0x31, 0x5d, 0x20e},
    {0x5e, 0x5f, 0x184},
    {0x60, 0x62, 0x23b},
    {0x66, 0x7e, 0x23e},
};

pdf_cmap_t cmap_HIRAGANA = {
    .name = "Hiragana",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 8,
    .char_range_map = char_range_map_HIRAGANA,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 1,
    .code_range_map = code_range_map_HIRAGANA,
    .next = NULL
};

