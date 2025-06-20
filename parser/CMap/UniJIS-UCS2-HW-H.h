#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_UNIJIS_UCS2_HW_H[] = {
    {0x0020, 0x005b, 0xe7},
    {0x005c, 0x005c, 0x220f},
    {0x005d, 0x007e, 0x124},
    {0x00a5, 0x00a5, 0x123},
};

pdf_cmap_t cmap_UNIJIS_UCS2_HW_H = {
    .name = "UniJIS-UCS2-HW-H",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 4,
    .char_range_map = char_range_map_UNIJIS_UCS2_HW_H,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

