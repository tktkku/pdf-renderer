#include "render.h"
void handle_Tj(pdf_context_t* context)
{
    // show / paint the glyphs for a string
    // string
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    if (context->state->textState.font == NULL)
        return;

    if (buf[0] == '<')
    {
        float x, y;
        plutovg_canvas_get_current_point(context->canvas, &x, &y);

        int unicode_cnt = 0;
        uint16_t unicode[1024] = { 0 };
        if (strstr(context->state->textState.font->encoding, "Identity"))
        {
            for (char* p = &buf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                unicode[unicode_cnt++] = t;
            }
        }
        else
        {
            for (char* p = &buf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                bool found = false;
                pdf_cmap_t* cmap = context->state->textState.font->cmap;
                while (cmap != NULL && !found)
                {
                    int nums = cvector_size(cmap->unicode_map);
                    for (int k = 0; k < nums; k++)
                    {
                        if (t == cmap->unicode_map[k]->cid)
                        {
                            unicode[unicode_cnt++] = cmap->unicode_map[k]->unicode;
                            found = true;
                            break;
                        }
                    }
                    nums = cvector_size(cmap->char_range_map);
                    for (int k = 0; k < nums && !found; k++)
                    {
                        if (t >= cmap->char_range_map[k]->dstStart &&
                            t <= cmap->char_range_map[k]->srcEnd)
                        {
                            unicode[unicode_cnt++] = cmap->char_range_map[k]->dstStart +
                                (t - cmap->char_range_map[k]->srcStart);
                            found = true;
                            break;
                        }
                    }
                    cmap = cmap->next;
                }
                if (!found)
                {
                    unicode[unicode_cnt++] = t;
                }
            }
        }

        plutovg_canvas_save(context->canvas);
        // cos(theta), sin(theta),  0
        // -sin(theta), cos(theta), 0
        // 0, 0, 1

        // 1,  0, 0
        // 0, -1, 0,
        // 0,  0, 1
        // rotate 180閹�?
        // or scale by 1
        plutovg_canvas_scale(context->canvas, 1, -1);
        plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
        plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
        plutovg_canvas_set_rgb(context->canvas,
            context->state->fillColor[0],
            context->state->fillColor[1],
            context->state->fillColor[2]);
        //printf("before %.2f\n", context->textState.lineWidth);
        if (context->state->textState.font_face_loaded)
        {
            if (strstr(context->state->textState.font->encoding, "Identity"))
                context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
            else
                context->state->textState.textLineWidth += plutovg_canvas_fill_text(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
        }
        else
            context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
        plutovg_canvas_restore(context->canvas);
        //printf("after %.2f\n", context->textState.lineWidth);
    }
    else if (buf[0] == '(')
    {
        int unicode_cnt = 0;
        uint16_t unicode[1024] = { 0 };
        if (!strcmp(context->state->textState.font->subtype, "/TrueType"))
        {
            for (int i = 1; i < node.size; i++)
            {
                unicode[unicode_cnt++] = buf[i];
            }
        }
        else
        {
            if (strstr(context->state->textState.font->encoding, "Identity"))
            {
                for (int i = 1; i < node.size; i += 2)
                {
                    unicode[unicode_cnt++] = ((buf[i] << 8) & 0xFF00) | (buf[i + 1] & 0x00FF);
                }
            }
            else
            {
                for (int i = 1; i < node.size; i += 2)
                {
                    uint16_t t = ((buf[i] << 8) & 0xFF00) | (buf[i + 1] & 0x00FF);
                    bool found = false;
                    pdf_cmap_t* cmap = context->state->textState.font->cmap;
                    while (cmap != NULL && !found)
                    {
                        int nums = cvector_size(cmap->unicode_map);
                        for (int k = 0; k < nums; k++)
                        {
                            if (t == cmap->unicode_map[k]->cid)
                            {
                                unicode[unicode_cnt++] = cmap->unicode_map[k]->unicode;
                                found = true;
                                break;
                            }
                        }
                        nums = cvector_size(cmap->char_range_map);
                        for (int k = 0; k < nums && !found; k++)
                        {
                            if (t >= cmap->char_range_map[k]->srcStart &&
                                t <= cmap->char_range_map[k]->srcEnd)
                            {
                                unicode[unicode_cnt++] = cmap->char_range_map[k]->dstStart +
                                    (t - cmap->char_range_map[k]->srcStart);
                                found = true;
                                break;
                            }
                        }
                        cmap = cmap->next;
                    }
                    if (!found)
                    {
                        unicode[unicode_cnt++] = t;
                    }
                }
            }
        }
        if (unicode_cnt == 0)
            return;

        plutovg_canvas_save(context->canvas);
        // cos(theta), sin(theta),  0
        // -sin(theta), cos(theta), 0
        // 0, 0, 1

        // 1,  0, 0
        // 0, -1, 0,
        // 0,  0, 1
        // rotate 180閹�?
        // or scale by 1
        plutovg_canvas_scale(context->canvas, 1, -1);
        plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
        plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
        plutovg_canvas_set_rgb(context->canvas,
            context->state->fillColor[0],
            context->state->fillColor[1],
            context->state->fillColor[2]);
        if (context->state->textState.font->load_succeed)
        {
            if (strstr(context->state->textState.font->encoding, "Identity")
                || strcmp(context->state->textState.font->encoding, "/WinAnsiEncoding") == 0)
                context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
            else
                context->state->textState.textLineWidth += plutovg_canvas_fill_text(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
        }
        else
            context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
        plutovg_canvas_restore(context->canvas);
    }
}