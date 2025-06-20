#include "../pdf-private.h"

static pdf_code_range_map_t code_range_map_HANKAKU[] = {
    {1, 0x00, 0xff},
};

static pdf_char_range_map_t char_range_map_HANKAKU[] = {
    {0x20, 0x5f, 0xe7},
    {0x60, 0x60, 0xe7},
    {0x61, 0x7e, 0x128},
    {0x81, 0x85, 0x147},
    {0x86, 0x8f, 0x204},
    {0x90, 0x90, 0x156},
    {0x91, 0x9f, 0x20e},
    {0xa1, 0xdf, 0x147},
    {0xe0, 0xfd, 0x21d},
    {0xfe, 0xff, 0x184},
};

pdf_cmap_t cmap_HANKAKU = {
    .name = "Hankaku",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 10,
    .char_range_map = char_range_map_HANKAKU,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 1,
    .code_range_map = code_range_map_HANKAKU,
    .next = NULL
};

