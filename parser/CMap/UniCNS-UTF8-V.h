#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNICNS_UTF8_V[] = {
    {0x78, 0xe28093},
    {0x7a, 0xe28094},
    {0x6d, 0xe280a5},
    {0x35b1, 0xefb98f},
    {0x86, 0xefbd9b},
    {0x87, 0xefbd9d},
};

static pdf_char_range_map_t char_range_map_UNICNS_UTF8_V[] = {
    {0xe38088, 0xe38089, 0x96},
    {0xe3808a, 0xe3808b, 0x92},
    {0xe3808c, 0xe3808d, 0x9a},
    {0xe3808e, 0xe3808f, 0x9e},
    {0xe38090, 0xe38091, 0x8e},
    {0xe38094, 0xe38095, 0x8a},
    {0xefbc88, 0xefbc89, 0x82},
};

pdf_cmap_t cmap_UNICNS_UTF8_V = {
    .name = "UniCNS-UTF8-V",
    .worldwide = true,
    .unicode_map_len = 6,
    .unicode_map = unicode_map_UNICNS_UTF8_V,
    .char_range_map_len = 7,
    .char_range_map = char_range_map_UNICNS_UTF8_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

