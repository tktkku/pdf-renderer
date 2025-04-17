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
    int nums = cvector_size(cmap->char_range_map);
    for (int i = 0; i < nums; i++)
    {
        free(cmap->char_range_map[i]);
    }
    cvector_free(cmap->char_range_map);
    cmap->char_range_map = NULL;
    nums = cvector_size(cmap->code_range_map);
    for (int i = 0; i < nums; i++)
    {
        free(cmap->code_range_map[i]);
    }
    cvector_free(cmap->code_range_map);
    cmap->code_range_map = NULL;
    nums = cvector_size(cmap->unicode_map);
    for (int i = 0; i < nums; i++)
    {
        free(cmap->unicode_map[i]);
    }
    cvector_free(cmap->unicode_map);
    cmap->unicode_map = NULL;
    free(cmap);
}
