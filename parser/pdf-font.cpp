#include "pdf-private.h"
#include "pdf.h"
#include <stdlib.h>
pdf_font_t* pdf_font_init()
{
    pdf_font_t* font = (pdf_font_t*)calloc(1, sizeof(pdf_font_t));

    return font;
}

void pdf_font_free(pdf_font_t* font)
{
    if (font == NULL)
        return;
    if (font->cmap != NULL)
    {
        pdf_cmap_t* p = font->cmap;
        pdf_cmap_t* q = font->cmap->next;
        while (q != NULL)
        {
            if (!q->worldwide)
            {
                p->next = q->next;
                pdf_cmap_free(q);
                q = p->next;
            }
            else 
            {
                p = p->next;
                q = p->next;
            }
        }
        if (font->cmap != NULL && !font->cmap->worldwide)
        {
            p = font->cmap;
            font->cmap = font->cmap->next;
            pdf_cmap_free(p);
        }
    }
    free(font);
    font = NULL;
}
