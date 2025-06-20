#include "../pdf-private.h"

static pdf_char_range_map_t char_range_map_ETENMS_B5_H[] = {
    {0x20, 0x7e, 0x1},
};

pdf_cmap_t cmap_ETENMS_B5_H = {
    .name = "ETenms-B5-H",
    .worldwide = true,
    .unicode_map_len = 0,
    .unicode_map = NULL,
    .char_range_map_len = 1,
    .char_range_map = char_range_map_ETENMS_B5_H,
    .not_def_range_len = 0,
    .not_def_range = NULL,
    .code_range_map_len = 0,
    .code_range_map = NULL,
    .next = NULL
};

