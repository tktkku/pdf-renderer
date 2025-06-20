#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIMANGA_UTF32_V[] = {
    {0x3cb3, 0x0000fe33},
};

static pdf_char_range_map_t char_range_map_UNIMANGA_UTF32_V[] = {
    {0x0000fe10, 0x0000fe19, 0x3ca6},
    {0x0000fe30, 0x0000fe31, 0x3cb0},
    {0x0000fe35, 0x0000fe44, 0x3cb5},
    {0x0000fe47, 0x0000fe48, 0x3cc7},
};

pdf_cmap_t cmap_UNIMANGA_UTF32_V = {
    .name = "UniManga-UTF32-V",
    .worldwide = true,
    .unicode_map_len = 1,
    .unicode_map = unicode_map_UNIMANGA_UTF32_V,
    .char_range_map_len = 4,
    .char_range_map = char_range_map_UNIMANGA_UTF32_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

