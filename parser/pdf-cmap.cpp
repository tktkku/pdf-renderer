#include "pdf-private.h"
#include <stdlib.h>
pdf_cmap_t* pdf_cmap_init()
{
    pdf_cmap_t* cmap = (pdf_cmap_t*)calloc(1, sizeof(pdf_cmap_t));
    return cmap;
}

void pdf_cmap_free(pdf_cmap_t* cmap)
{
    if (cmap == NULL)
        return;
    if (cmap->char_range_map)
        free(cmap->char_range_map);
    if (cmap->code_range_map)
        free(cmap->code_range_map);
    if (cmap->unicode_map)
        free(cmap->unicode_map);
    free(cmap);
}

