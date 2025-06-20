#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIMANGA_UTF16_V[] = {
    {0x3cb3, 0xfe33},
};

static pdf_char_range_map_t char_range_map_UNIMANGA_UTF16_V[] = {
    {0xfe10, 0xfe19, 0x3ca6},
    {0xfe30, 0xfe31, 0x3cb0},
    {0xfe35, 0xfe44, 0x3cb5},
    {0xfe47, 0xfe48, 0x3cc7},
};

pdf_cmap_t cmap_UNIMANGA_UTF16_V = {
    .name = "UniManga-UTF16-V",
    .worldwide = true,
    .unicode_map_len = 1,
    .unicode_map = unicode_map_UNIMANGA_UTF16_V,
    .char_range_map_len = 4,
    .char_range_map = char_range_map_UNIMANGA_UTF16_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

