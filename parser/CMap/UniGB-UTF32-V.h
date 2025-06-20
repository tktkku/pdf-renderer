#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_UNIGB_UTF32_V[] = {
    {0x256, 0x00002014},
    {0x257, 0x00002026},
    {0x23f, 0x00003001},
    {0x23e, 0x00003002},
    {0x1e1a, 0x00003013},
    {0x242, 0x0000ff01},
    {0x23d, 0x0000ff0c},
    {0x1e1b, 0x0000ff0e},
    {0x1e1c, 0x0000ff1d},
    {0x243, 0x0000ff1f},
    {0x1e1d, 0x0000ff3b},
    {0x1e1e, 0x0000ff3d},
    {0x258, 0x0000ff3f},
    {0x254, 0x0000ff5b},
    {0x255, 0x0000ff5d},
    {0x1e18, 0x0000ff5e},
    {0x1e1f, 0x0000ffe3},
};

static pdf_char_range_map_t char_range_map_UNIGB_UTF32_V[] = {
    {0x00003008, 0x0000300f, 0x248},
    {0x00003010, 0x00003011, 0x252},
    {0x00003014, 0x00003015, 0x246},
    {0x00003016, 0x00003017, 0x250},
    {0x0000ff08, 0x0000ff09, 0x244},
    {0x0000ff1a, 0x0000ff1b, 0x240},
};

pdf_cmap_t cmap_UNIGB_UTF32_V = {
    .name = "UniGB-UTF32-V",
    .worldwide = true,
    .unicode_map_len = 17,
    .unicode_map = unicode_map_UNIGB_UTF32_V,
    .char_range_map_len = 6,
    .char_range_map = char_range_map_UNIGB_UTF32_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

