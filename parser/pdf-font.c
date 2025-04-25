#include "pdf-private.h"
#include "pdf.h"
#include <stdlib.h>
pdf_font_t* pdf_font_init()
{
    pdf_font_t* font = (pdf_font_t*)calloc(1, sizeof(pdf_font_t));
    font->references = 1;
    return font;
}
pdf_font_t* pdf_font_reference(pdf_font_t* font)
{
    if (font == NULL) return NULL;
    font->references++;
    return font;
}
void pdf_font_free(pdf_font_t* font)
{
    if (font == NULL)
        return;
    font->references--;
    if (font->references == 0)
    {
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
        if (font->to_unicode_map != NULL)
        {
            pdf_cmap_t* p = font->to_unicode_map;
            pdf_cmap_t* q = font->to_unicode_map->next;
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
            if (font->to_unicode_map != NULL && !font->to_unicode_map->worldwide)
            {
                p = font->to_unicode_map;
                font->to_unicode_map = font->to_unicode_map->next;
                pdf_cmap_free(p);
            }
        }
        if (font->differences != NULL)
        {
            pdf_array_free(font->differences);
        }
        if (font->charstrings != NULL)
        {
            pdf_array_free(font->charstrings);
        }
        if (font->global_subr != NULL)
        {
            pdf_array_free(font->global_subr);
        }
        if (font->font_dict_arr != NULL)
        {
            pdf_array_free(font->font_dict_arr);
        }
        if (font->font_dict_select_arr != NULL)
        {
            pdf_array_free(font->font_dict_select_arr);
        }
        free(font);
        font = NULL;
    }
}
