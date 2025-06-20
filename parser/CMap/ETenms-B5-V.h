#include "../pdf-private.h"

static pdf_unicode_map_t unicode_map_ETENMS_B5_V[] = {
    {0x354e, 0xa14b},
    {0x6d, 0xa14c},
    {0x138, 0xa156},
    {0x7a, 0xa158},
    {0x35af, 0xa15a},
    {0x35b1, 0xa15c},
};

static pdf_char_range_map_t char_range_map_ETENMS_B5_V[] = {
    {0xa15d, 0xa15e, 0x82},
    {0xa161, 0xa162, 0x86},
    {0xa165, 0xa166, 0x8a},
    {0xa169, 0xa16a, 0x8e},
    {0xa16d, 0xa16e, 0x92},
    {0xa171, 0xa172, 0x96},
    {0xa175, 0xa176, 0x9a},
    {0xa179, 0xa17a, 0x9e},
    {0xa17d, 0xa17e, 0x82},
    {0xa1a1, 0xa1a2, 0x86},
    {0xa1a3, 0xa1a4, 0x8a},
    {0xc6e4, 0xc6e5, 0x3711},
};

pdf_cmap_t cmap_ETENMS_B5_V = {
    .name = "ETenms-B5-V",
    .worldwide = true,
    .unicode_map_len = 6,
    .unicode_map = unicode_map_ETENMS_B5_V,
    .char_range_map_len = 12,
    .char_range_map = char_range_map_ETENMS_B5_V,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

