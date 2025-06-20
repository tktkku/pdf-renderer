#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_UNICNS_UCS2_V[] = {
    {0x2013, 0x2013, 0x78},
    {0x2014, 0x2014, 0x7a},
    {0x2025, 0x2025, 0x6d},
    {0x3008, 0x3009, 0x96},
    {0x300a, 0x300b, 0x92},
    {0x300c, 0x300d, 0x9a},
    {0x300e, 0x300f, 0x9e},
    {0x3010, 0x3011, 0x8e},
    {0x3014, 0x3015, 0x8a},
    {0xfe4f, 0xfe4f, 0x35b1},
    {0xff08, 0xff09, 0x82},
    {0xff5b, 0xff5b, 0x86},
    {0xff5d, 0xff5d, 0x87},
};

pdf_cmap_t cmap_UNICNS_UCS2_V = {
    .name = "UniCNS-UCS2-V",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 13,
    .char_range_map = char_range_map_UNICNS_UCS2_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

