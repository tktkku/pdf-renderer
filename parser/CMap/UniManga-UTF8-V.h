#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIMANGA_UTF8_V[] = {
    {0x3cb3, 0xefb8b3},
};

static pdf_char_range_map_t char_range_map_UNIMANGA_UTF8_V[] = {
    {0xefb890, 0xefb899, 0x3ca6},
    {0xefb8b0, 0xefb8b1, 0x3cb0},
    {0xefb8b5, 0xefb8bf, 0x3cb5},
    {0xefb980, 0xefb984, 0x3cc0},
    {0xefb987, 0xefb988, 0x3cc7},
};

pdf_cmap_t cmap_UNIMANGA_UTF8_V = {
    .name = "UniManga-UTF8-V",
    .worldwide = true,
    .unicode_map_len = 1,
    .unicode_map = unicode_map_UNIMANGA_UTF8_V,
    .char_range_map_len = 5,
    .char_range_map = char_range_map_UNIMANGA_UTF8_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

