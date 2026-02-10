#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
void handle_Tf(pdf_render_t* context)
{
    // set font and font size to use
    // fontname fontsize
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float fontsize = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);

    pdf_font_t* font = pdf_obj_get_font(context->current_obj, buf);
    context->state->textState.fontSize = fontsize;
    if (font == NULL)
        return;
    
    for (int i = 0; i < cvector_size(context->fontcache); i++)
    {
        pdf_font_cache_t* cache = context->fontcache[i];
        if (cache->font == font)
        {
            context->state->textState.fontface = cache->fontface;
            context->state->textState.font_face_loaded = cache->loaded;
            context->state->textState.font = font;
            //plutovg_canvas_set_font(context->canvas, context->state->textState.fontface, fontsize);
            return;
        }
    }
    
    // plutovg_font_face_destroy(context->state->textState.fontface);
    // plutovg_canvas_set_font_face(context->canvas, NULL);
    // context->state->textState.fontface = NULL;
    context->state->textState.font = font;
    

    // repair font
    // repair_cmap(context->font);
    // set font face
    // context->fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf",
    // 0); FT_Face face;
    // context->state->textState.font_face_loaded = false;
    // if (font->subtype && strcmp(font->subtype, "/TrueType") == 0)
    // {
    //     if (font->font_data == NULL)
    //     {
    //         context->state->textState.fontface = NULL;
    //         for (int i = 0; i < cvector_size(context->page->pdf->external_fonts); i++)
    //         {
    //             pdf_external_font_t* f = context->page->pdf->external_fonts[i];
    //             if (strcmp(f->name, font->basefont + 1) == 0)
    //             {
    //                 font->font_data_length = f->data_len;
    //                 font->font_data = f->data;
                    
    //                 context->state->textState.fontface = plutovg_font_face_load_from_data(
    //                     font->font_data, font->font_data_length, 0, NULL, NULL);
    //                 context->state->textState.font_face_loaded = true;
    //                 break;
    //             }
    //         }
    //         context->state->textState.font_face_loaded = true;
    //     }
    //     else
    //     {
    //         context->state->textState.fontface = plutovg_font_face_load_from_data(
    //             font->font_data, font->font_data_length, 0, NULL, NULL);
    //         context->state->textState.font_face_loaded = true;
    //     }
    // }
    // else
    if (font->subtype == FONT_SUBTYPE_TYPE0)
    {
        pdf_font_cidfont_t* cidfont = font->type0->descendant->cidfont;
        if (cidfont->font_descriptor->fontfile != NULL)
        {
            if ((context->state->textState.fontface = plutovg_font_face_load_from_data(
                cidfont->font_descriptor->fontfile, cidfont->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
            {
                context->state->textState.font_face_loaded = false;
                context->state->textState.fontface = plutovg_font_face_load_from_data1(
                    cidfont->font_descriptor->fontfile, cidfont->font_descriptor->fontfile_len, 0, NULL, NULL);
            }
            else
            {
                context->state->textState.font_face_loaded = true;
            }
        }
        else
        {
            context->state->textState.font_face_loaded = true;
            context->state->textState.fontface = plutovg_font_face_load_from_file("fonts/NotoSerifSC-Regular.ttf", 0);
        }
    }
    else if (font->subtype == FONT_SUBTYPE_TRUETYPE || font->subtype == FONT_SUBTYPE_TYPE1)
    {
        pdf_font_type1_t* type1_truetype = font->type1_truetype;
        if (type1_truetype->font_descriptor->fontfile != NULL)
        {
            if ((context->state->textState.fontface = plutovg_font_face_load_from_data(
                type1_truetype->font_descriptor->fontfile, type1_truetype->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
            {
                context->state->textState.font_face_loaded = false;
                context->state->textState.fontface = plutovg_font_face_load_from_data1(
                    type1_truetype->font_descriptor->fontfile, type1_truetype->font_descriptor->fontfile_len, 0, NULL, NULL);
            }
            else
            {
                context->state->textState.font_face_loaded = true;
            }
        }
    }
    pdf_font_cache_t* cache = (pdf_font_cache_t*)malloc(sizeof(pdf_font_cache_t));
    cache->font = pdf_font_reference(font);
    cache->fontface = context->state->textState.fontface;
    cache->loaded = context->state->textState.font_face_loaded;
    cvector_push_back(context->fontcache, cache);
}