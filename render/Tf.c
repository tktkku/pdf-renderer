#include "render.h"
void handle_Tf(pdf_context_t* context)
{
    // set font and font size to use
    // fontname fontsize
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float fontsize = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    pdf_font_t* font = pdf_page_get_font(context->page, buf);
    context->state->textState.fontSize = fontsize;
    if (font == NULL)
        return;
    else
    {
        //pdf_font_free(context->state->textState.font);
        context->state->textState.font = font;
        plutovg_canvas_set_font_face(context->canvas, NULL);
        plutovg_font_face_destroy(context->state->textState.fontface);
        context->state->textState.fontface = NULL;
    }

    // repair font
    // repair_cmap(context->font);
    // set font face
    // context->fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf",
    // 0); FT_Face face;
    context->state->textState.font_face_loaded = false;
    if (strcmp(font->subtype, "/TrueType") == 0)
    {
        if (font->font_data == NULL)
        {
            if (strcmp(font->basefont, "/SimSun") == 0)
            {
                context->state->textState.fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf", 0);
                context->state->textState.font_face_loaded = true;
            }
            else
            {
                // FT_New_Face(context->ft_library, "fonts/SimSun.ttf", 0, &face);
                context->state->textState.fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf", 0);
                context->state->textState.font_face_loaded = true;
            }
        }
        else
        {
            context->state->textState.fontface = plutovg_font_face_load_from_data(
                font->font_data, font->font_data_length, 0, NULL, NULL);
            context->state->textState.font_face_loaded = true;
        }
    }
    else
    {
        if (font->font_data == NULL)
        {
            // FT_New_Face(context->ft_library, "fonts/SimSun.ttf", 0, &face);
            context->state->textState.fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf", 0);
            context->state->textState.font_face_loaded = true;
        }
        else
        {
            // FT_New_Memory_Face(context->ft_library, font->font_data,
            // font->font_data_length, 0, &face);
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

    plutovg_canvas_set_font(context->canvas, context->state->textState.fontface, fontsize);
    // set font matrix
    // plutovg_matrix_init_scale(&context->fontMatrixPlutovg, fontsize, fontsize);
    // plutovg_canvas_set_matrix(context->canvas, &context->fontMatrixPlutovg);
}