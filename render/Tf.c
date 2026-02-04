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

    pdf_font_t* font = pdf_obj_get_font(context->current_obj, buf);;
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
    context->state->textState.font_face_loaded = false;
    if (font->subtype && strcmp(font->subtype, "/TrueType") == 0)
    {
        if (font->font_data == NULL)
        {
            context->state->textState.fontface = NULL;
            for (int i = 0; i < cvector_size(context->page->pdf->external_fonts); i++)
            {
                pdf_external_font_t* f = context->page->pdf->external_fonts[i];
                if (strcmp(f->name, font->basefont + 1) == 0)
                {
                    font->font_data_length = f->data_len;
                    font->font_data = f->data;
                    
                    context->state->textState.fontface = plutovg_font_face_load_from_data(
                        font->font_data, font->font_data_length, 0, NULL, NULL);
                    context->state->textState.font_face_loaded = true;
                    break;
                }
            }
            context->state->textState.font_face_loaded = true;
        }
        else
        {
            context->state->textState.fontface = plutovg_font_face_load_from_data(
                font->font_data, font->font_data_length, 0, NULL, NULL);
            context->state->textState.font_face_loaded = true;
        }
    }
    else if (font->subtype && strcmp(font->subtype, "/CIDFontType2") == 0)
    {
        if (font->cid_system_info.registry != NULL && !strcmp(font->cid_system_info.registry, "Adobe")
        && font->cid_system_info.ordering != NULL && !strcmp(font->cid_system_info.ordering, "GB1"))
        {
            
        }
    }
    else
    {
        if (font->font_data == NULL)
        {
            context->state->textState.fontface = NULL;
            for (int i = 0; i < cvector_size(context->page->pdf->external_fonts); i++)
            {
                pdf_external_font_t* f = context->page->pdf->external_fonts[i];
                if (strcmp(f->name, font->basefont + 1) == 0)
                {
                    font->font_data_length = f->data_len;
                    font->font_data = f->data;
                    
                    context->state->textState.fontface = plutovg_font_face_load_from_data(
                        font->font_data, font->font_data_length, 0, NULL, NULL);
                    context->state->textState.font_face_loaded = true;
                    break;
                }
            }
            context->state->textState.font_face_loaded = true;
        }
        else
        {
            if ((context->state->textState.fontface = plutovg_font_face_load_from_data(
                font->font_data, font->font_data_length, 0, NULL, NULL)) == NULL)
            {
                context->state->textState.font_face_loaded = false;
                context->state->textState.fontface = plutovg_font_face_load_from_data1(
                    font->font_data, font->font_data_length, 0, NULL, NULL);
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
    // set font face
    //plutovg_canvas_set_font(context->canvas, context->state->textState.fontface, fontsize);
    // set font matrix
    // plutovg_matrix_init_scale(&context->fontMatrixPlutovg, fontsize, fontsize);
    // plutovg_canvas_set_matrix(context->canvas, &context->fontMatrixPlutovg);
}