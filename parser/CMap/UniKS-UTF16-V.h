#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIKS_UTF16_V[] = {
    {0x1f7d, 0x2016},
    {0x1f7a, 0x2025},
    {0x1f8b, 0x3013},
    {0x1f8c, 0xff01},
    {0x1f8f, 0xff0c},
    {0x1f90, 0xff0e},
    {0x1f97, 0xff3b},
    {0x1f98, 0xff3d},
    {0x1f99, 0xff3f},
    {0x1f7e, 0xff5e},
    {0x1f9d, 0xffe3},
};

static pdf_char_range_map_t char_range_map_UNIKS_UTF16_V[] = {
    {0x2013, 0x2014, 0x1f7b},
    {0x3001, 0x3002, 0x1f78},
    {0x3008, 0x3011, 0x1f81},
    {0x3014, 0x3015, 0x1f7f},
    {0xff08, 0xff09, 0x1f8d},
    {0xff1a, 0xff1f, 0x1f91},
    {0xff5b, 0xff5d, 0x1f9a},
};

pdf_cmap_t cmap_UNIKS_UTF16_V = {
    .name = "UniKS-UTF16-V",
    .worldwide = true,
    .unicode_map_len = 11,
    .unicode_map = unicode_map_UNIKS_UTF16_V,
    .char_range_map_len = 7,
    .char_range_map = char_range_map_UNIKS_UTF16_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

