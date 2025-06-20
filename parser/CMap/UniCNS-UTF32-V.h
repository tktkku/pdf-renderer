#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNICNS_UTF32_V[] = {
    {0x78, 0x00002013},
    {0x7a, 0x00002014},
    {0x6d, 0x00002025},
    {0x35b1, 0x0000fe4f},
    {0x86, 0x0000ff5b},
    {0x87, 0x0000ff5d},
};

static pdf_char_range_map_t char_range_map_UNICNS_UTF32_V[] = {
    {0x00003008, 0x00003009, 0x96},
    {0x0000300a, 0x0000300b, 0x92},
    {0x0000300c, 0x0000300d, 0x9a},
    {0x0000300e, 0x0000300f, 0x9e},
    {0x00003010, 0x00003011, 0x8e},
    {0x00003014, 0x00003015, 0x8a},
    {0x0000ff08, 0x0000ff09, 0x82},
};

pdf_cmap_t cmap_UNICNS_UTF32_V = {
    .name = "UniCNS-UTF32-V",
    .worldwide = true,
    .unicode_map_len = 6,
    .unicode_map = unicode_map_UNICNS_UTF32_V,
    .char_range_map_len = 7,
    .char_range_map = char_range_map_UNICNS_UTF32_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

