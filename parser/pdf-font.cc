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
void _free_cmap(pdf_cmap_t* cmap)
{
    if (cmap == NULL) return;
    pdf_cmap_t* p = cmap;
    pdf_cmap_t* q = cmap->next;
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
    if (cmap != NULL && !cmap->worldwide)
    {
        p = cmap;
        cmap = cmap->next;
        pdf_cmap_free(p);
    }
}
void pdf_font_free(pdf_font_t* font)
{
    if (font == NULL)
        return;
    font->references--;
    if (font->references == 0)
    {
        if (font->subtype == FONT_SUBTYPE_TYPE0)
        {
            _free_cmap(font->type0->encoding);
            _free_cmap(font->type0->to_unicode_map);
            pdf_font_free(font->type0->descendant);
            free(font->type0);
        }
        else if (font->subtype == FONT_SUBTYPE_TYPE1 || font->subtype == FONT_SUBTYPE_TRUETYPE)
        {
            _free_cmap(font->type1_truetype->to_unicode_map);
            pdf_array_free(font->type1_truetype->differences);
            free(font->type1_truetype->font_descriptor);
            free(font->type1_truetype);
        }
        else if (font->subtype == FONT_SUBTYPE_TYPE3)
        {
            _free_cmap(font->type3->to_unicode_map);
            pdf_array_free(font->type3->differences);
            free(font->type3->font_descriptor);
            free(font->type3);
        }
        else if (font->subtype == FONT_SUBTYPE_CIDFONTTPYE0 || font->subtype == FONT_SUBTYPE_CIDFONTTPYE2)
        {
            free(font->cidfont->cid_system_info.registry);
            free(font->cidfont->cid_system_info.ordering);
            _free_cmap(font->cidfont->cid_to_gid_map);
            free(font->cidfont->font_descriptor);
            free(font->cidfont);
        }
        free(font);
        font = NULL;
    }
}
