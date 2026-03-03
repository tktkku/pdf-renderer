#include "pdf.h"
#include "pdf-private.h"

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
        if (font->subtype == FONT_SUBTYPE_TYPE0)
        {
            if (font->type0->encoding && !font->type0->encoding->isGlobal)
            {
                delete font->type0->encoding;
            }
            if (font->type0->to_unicode_map && !font->type0->to_unicode_map->isGlobal)
            {
                delete font->type0->to_unicode_map;
            }
            pdf_font_free(font->type0->descendant);
            free(font->type0);
        }
        else if (font->subtype == FONT_SUBTYPE_TYPE1 || font->subtype == FONT_SUBTYPE_TRUETYPE)
        {
            if (font->type1_truetype->to_unicode_map && !font->type1_truetype->to_unicode_map->isGlobal)
            {
                delete font->type1_truetype->to_unicode_map;
            }
            delete font->type1_truetype->differences;
            free(font->type1_truetype->font_descriptor);
            free(font->type1_truetype);
        }
        else if (font->subtype == FONT_SUBTYPE_TYPE3)
        {
            if (font->type3->to_unicode_map && !font->type3->to_unicode_map->isGlobal)
            {
                delete font->type3->to_unicode_map;
            }
            delete font->type3->differences;
            free(font->type3->font_descriptor);
            free(font->type3);
        }
        else if (font->subtype == FONT_SUBTYPE_CIDFONTTPYE0 || font->subtype == FONT_SUBTYPE_CIDFONTTPYE2)
        {
            free(font->cidfont->cid_system_info.registry);
            free(font->cidfont->cid_system_info.ordering);
            if (font->cidfont->cid_to_gid_map && !font->cidfont->cid_to_gid_map->isGlobal)
            {
                delete font->cidfont->cid_to_gid_map;
            }
            free(font->cidfont->font_descriptor);
            free(font->cidfont);
        }
        free(font);
        font = NULL;
    }
}
