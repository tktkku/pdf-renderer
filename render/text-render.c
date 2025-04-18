#include "render.h"
#include "pdf-private.h"
#include "plutovg-private.h"
uint16_t _get_unicode_from_cmap(pdf_cmap_t* cmap, uint16_t code)
{
    bool found = false;
    while (cmap != NULL && !found)
    {
        int nums = cvector_size(cmap->unicode_map);
        for (int k = 0; k < nums; k++)
        {
            if (code == cmap->unicode_map[k]->cid)
            {
                return cmap->unicode_map[k]->unicode;
            }
        }
        nums = cvector_size(cmap->char_range_map);
        for (int k = 0; k < nums && !found; k++)
        {
            if (code >= cmap->char_range_map[k]->dstStart &&
                code <= cmap->char_range_map[k]->srcEnd)
            {
                return cmap->char_range_map[k]->dstStart +
                    (code - cmap->char_range_map[k]->srcStart);
            }
        }
        cmap = cmap->next;
    }
    return code;
}
static void glyph_traverse_func(void* closure, plutovg_path_command_t command, const plutovg_point_t* points, int npoints)
{
    plutovg_path_t* path = (plutovg_path_t*)(closure);
    switch(command) {
    case PLUTOVG_PATH_COMMAND_MOVE_TO:
        plutovg_path_move_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_LINE_TO:
        plutovg_path_line_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_CUBIC_TO:
        plutovg_path_cubic_to(path, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case PLUTOVG_PATH_COMMAND_CLOSE:
        plutovg_path_close(path);
        break;
    }
}

float _canvas_fill_text(pdf_context_t* context, const void* text, int length, plutovg_text_encoding_t encoding, float x, float y, bool cid_eq_gid)
{
    plutovg_canvas_t* canvas = context->canvas; 
    plutovg_canvas_new_path(canvas);
    pdf_graphics_state_t* state = context->state;
    if (state->textState.ft_face == NULL || state->textState.fontSize <= 0.f)
        return 0.f;

    FT_Face face = state->textState.ft_face;
    FT_Set_Char_Size(face, 0, (FT_F26Dot6)(state->textState.fontSize), 0, 0);

    plutovg_text_iterator_t it;
    plutovg_text_iterator_init(&it, text, length, encoding);
    float advance_width = 0.f;

    while (plutovg_text_iterator_has_next(&it)) {
        plutovg_codepoint_t codepoint = plutovg_text_iterator_next(&it);
        if (cid_eq_gid)
        {
            if (FT_Load_Char(face, codepoint, FT_LOAD_NO_SCALE)) {
                continue; // Skip invalid glyphs
            }
        }
        else
        {
            FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
            FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE);
        }

        FT_GlyphSlot slot = face->glyph;
        FT_Outline* outline = &slot->outline;

        float scale = state->textState.fontSize / face->units_per_EM;
        plutovg_matrix_t matrix;
        plutovg_matrix_init_translate(&matrix, x, y);
        plutovg_matrix_scale(&matrix, scale, -scale);

        plutovg_path_t* path = canvas->path;
        for (int i = 0; i < outline->n_contours; i++) {
            int start = (i == 0) ? 0 : outline->contours[i - 1] + 1;
            int end = outline->contours[i];

            for (int j = start; j <= end; j++) {
                FT_Vector* point = &outline->points[j];
                char tag = outline->tags[j];

                plutovg_point_t mapped_point = { point->x, point->y};
                plutovg_matrix_map_points(&matrix, &mapped_point, &mapped_point, 1);

                if (tag & FT_CURVE_TAG_ON) {
                    if (j == start) {
                        glyph_traverse_func(path, PLUTOVG_PATH_COMMAND_MOVE_TO, &mapped_point, 1);
                    } else {
                        glyph_traverse_func(path, PLUTOVG_PATH_COMMAND_LINE_TO, &mapped_point, 1);
                    }
                } 
                else if (tag & FT_CURVE_TAG_CONIC)
                {
                    FT_Vector* next_point = &outline->points[(j + 1) % (end + 1)];
                    plutovg_point_t control = { point->x, point->y};
                    plutovg_point_t end_point = { next_point->x, next_point->y};

                    plutovg_matrix_map_points(&matrix, &control, &control, 1);
                    plutovg_matrix_map_points(&matrix, &end_point, &end_point, 1);

                    glyph_traverse_func(path, PLUTOVG_PATH_COMMAND_CUBIC_TO, (plutovg_point_t[]){ control, control, end_point }, 3);
                }
                else if (tag & FT_CURVE_TAG_CUBIC) {
                    FT_Vector* next_point = &outline->points[(j + 1) % (end + 1)];
                    plutovg_point_t control1 = { point->x, point->y};
                    plutovg_point_t control2 = { next_point->x, next_point->y};
                    plutovg_point_t end_point = { outline->points[(j + 2) % (end + 1)].x, outline->points[(j + 2) % (end + 1)].y};

                    plutovg_matrix_map_points(&matrix, &control1, &control1, 1);
                    plutovg_matrix_map_points(&matrix, &control2, &control2, 1);
                    plutovg_matrix_map_points(&matrix, &end_point, &end_point, 1);

                    glyph_traverse_func(path, PLUTOVG_PATH_COMMAND_CUBIC_TO, (plutovg_point_t[]){ control1, control2, end_point }, 3);
                    j += 2; // Skip the next two points
                }
            }
            glyph_traverse_func(path, PLUTOVG_PATH_COMMAND_CLOSE, NULL, 0);
        }

        advance_width += slot->advance.x * scale;
        x += slot->advance.x * scale;
    }

    plutovg_canvas_fill(canvas);
    return advance_width;
}

void _cff_do_render_char(pdf_context_t* context, pdf_deque_t* deque, unsigned char* buf, int len)
{
    plutovg_canvas_t* canvas = context->canvas;
    pdf_array_t* charstrings_index = context->state->textState.font->charstrings;
    pdf_array_t* global_subr_index = context->state->textState.font->global_subr;
    uint16_t global_bias = context->state->textState.font->global_subr_bias;

    for (int i = 0; i < len; i++)
    {
        printf("%d ", buf[i]);
    }
    printf("\n");
    unsigned char* p = buf;
    unsigned char* end = buf + len;
    pdf_node_t node;
    void* data = malloc(16);
    node.data = data;
    static double width = 0;
    while (p < end)
    {
        double v = 0;
        uint8_t b0 = p[0];
        if (b0 == 28)
        {
            p++;
            uint8_t b1 = p[0];
            uint8_t b2 = p[1];
            p += 2;
            v = b1 << 8 | b2; 
        }
        else if (b0 >= 32 && b0 <= 246)
        { 
            p++;
            v = b0 - 139;
        }
        else if (b0 >= 247 && b0 <= 250)
        {
            p++;
            uint8_t b1 = p[0];
            p++;
            v = (b0 - 247) * 256 + b1 + 108;
        }
        else if (b0 >= 251 && b0 <= 254)
        {
            p++;
            uint8_t b1 = p[0];
            p++;
            v = -(b0 - 251) * 256 - b1 - 108;
        }
        else if (b0 == 255)
        {
            p++;
            int32_t raw_value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
            int32_t signed_value = (int32_t)raw_value;
            v = (float)signed_value / 65536.0;
            p += 4;
        } 
        else if (b0 == 11 || b0 == 14)
        {
            break;
        }
        pdf_deque_push(deque, &v, sizeof(double));
        printf("%d ", (uint32_t)v);
        uint8_t operator1 = p[0];
        switch (operator1)
        {
            case 1://hstem
            case 3://vstem
            case 4://vmoveto
            case 5://rlineto
            case 6://hlineto
            case 7://vlineto
            case 8://rrcurveto
            case 10://callsubr
            case 18://hstemhm
            case 19://hintmask
            case 20://cntrmask
            case 21://rmoveto
            case 22://hmoveto
            case 23://vstemhm
            case 24://rcurveline
            case 25://rlinecurve
            case 26://vvcurveto
            case 27://hhcurveto
            case 29://callgsubr
            case 30://vhcurveto
            case 31://hvcurveto
            {
                CFF_HANDLERS1[operator1](context, deque);
                p++;
                break;
            }
            case 11://return
            case 14://endcar
            {
                free(data);
                return;
            }
            case 12:
            {
                uint8_t operator2 = p[1];
                switch (operator2)
                {
                    case 3: // and
                    case 4: // or
                    case 5:// not
                    case 9://abs
                    case 10: // add
                    case 11: // sub
                    case 12: // div
                    case 14: // neg
                    case 15://eq
                    case 18: // drop
                    case 20: // put
                    case 21: // get
                    case 22:// ifelse
                    case 23://random
                    case 24://mul
                    case 26: // sqrt
                    case 27: //dup
                    case 28: //exch
                    case 29: // index
                    case 30:// roll
                    case 34://hflex
                    case 35://flex
                    case 36: // hflex1
                    case 37://flex1
                    {
                        CFF_HANDLERS2[operator2](context, deque);
                        p += 2;
                        break;
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
    }
    free(data);
}

void _do_text_render(pdf_context_t* context, char* buf, int len)
{
    plutovg_canvas_save(context->canvas);
    int unicode_cnt = 0;
    uint16_t unicode[1024] = { 0 };
    unsigned char* pbuf = buf;
    if (pbuf[0] == '<')
    {
        if (strstr(context->state->textState.font->encoding, "Identity"))
        {
            for (char* p = &pbuf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                unicode[unicode_cnt++] = t;
            }
        }
        else
        {
            for (char* p = &pbuf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                unicode[unicode_cnt++] = _get_unicode_from_cmap(context->state->textState.font->cmap, t);
            }
        }
        // for (char* p = &pbuf[1]; *p != '\0'; p += 4)
        // {
        //     uint16_t t = _hex_str_to_16bit(p);
        //     wchar_t w = _get_unicode_from_cmap(context->state->textState.font->to_unicode_map, t);
        //     printf("%lc", w);
        // }
        // printf("\n");
    }
    else if (pbuf[0] == '(')
    {
        if (!strcmp(context->state->textState.font->subtype, "/TrueType") || !strcmp(context->state->textState.font->subtype, "/Type3"))
        {
            for (int i = 1; i < len; i++)
            {
                unicode[unicode_cnt++] = pbuf[i];
            }
        }
        else
        {
            if (strstr(context->state->textState.font->encoding, "Identity"))
            {
                for (int i = 1; i < len; i += 2)
                {
                    unicode[unicode_cnt++] = ((pbuf[i] << 8) & 0xFF00) | (pbuf[i + 1] & 0x00FF);
                }
            }
            else
            {
                for (int i = 1; i < len; i += 2)
                {
                    uint16_t t = ((pbuf[i] << 8) & 0xFF00) | (pbuf[i + 1] & 0x00FF);
                    unicode[unicode_cnt++] = _get_unicode_from_cmap(context->state->textState.font->cmap, t);
                }
            }
        }
    }
    if (unicode_cnt == 0)
    {
        plutovg_canvas_restore(context->canvas);
        return;
    }

    if (strcmp(context->state->textState.font->subtype, "/Type3"))
    {
#if USE_FREETYPE
        if (strstr(context->state->textState.font->encoding, "Identity"))
            context->state->textState.textLineWidth += _canvas_fill_text(context, unicode, unicode_cnt,
            PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0, true);
        else
        context->state->textState.textLineWidth += _canvas_fill_text(context, unicode, unicode_cnt,
            PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0, false);
#else
        if (context->state->textState.fontface != NULL)
        {
            float x, y;
            plutovg_canvas_get_current_point(context->canvas, &x, &y);
            // cos(theta), sin(theta),  0
            // -sin(theta), cos(theta), 0
            // 0, 0, 1

            // 1,  0, 0
            // 0, -1, 0,
            // 0,  0, 1
            // rotate 180閹�?
            // or scale by 1
            plutovg_canvas_transform(context->canvas, &context->state->textState.textMatrix);
            plutovg_canvas_scale(context->canvas, 1, -1);
            plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
            plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
            plutovg_canvas_set_rgb(context->canvas,
                context->state->fill.color[0],
                context->state->fill.color[1],
                context->state->fill.color[2]);
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
            {
                context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
            }
        }
        else
        {
            pdf_array_t* charstrings_index = context->state->textState.font->charstrings;

            if (charstrings_index != NULL)
            {
                // context->state->textState.textLineWidth += _canvas_fill_text(context, unicode, unicode_cnt,
                //     PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0, true);
                pdf_array_t* font_matrix = context->state->textState.font->font_matrix;
                for (int i = 0; i < unicode_cnt; i++)
                {
                    if (unicode[i] != 26) continue;
                    pdf_deque_t* deque = pdf_deque_init();
                    plutovg_canvas_save(context->canvas);
                    plutovg_matrix_t font_matrix_plutovg, rm;
                    plutovg_matrix_init(&font_matrix_plutovg, 
                        font_matrix->values[0]->val.number, font_matrix->values[1]->val.number,
                        font_matrix->values[2]->val.number, font_matrix->values[3]->val.number,
                        font_matrix->values[4]->val.number, font_matrix->values[5]->val.number);
                    plutovg_matrix_multiply(&rm, &font_matrix_plutovg, &context->state->textState.textMatrix);
                    plutovg_matrix_t m = {
                        context->state->textState.fontSize * context->state->textState.horizontalScaling / 100, 0,
                        0, context->state->textState.fontSize,
                        0, context->state->textState.textRise
                    };
                    plutovg_matrix_multiply(&rm, &m, &rm);
                    plutovg_canvas_transform(context->canvas, &rm);
                    plutovg_canvas_set_rgb(context->canvas,
                        context->state->fill.color[0],
                        context->state->fill.color[1],
                        context->state->fill.color[2]);
                    plutovg_canvas_new_path(context->canvas);

                    int len = charstrings_index->values[unicode[i]]->value_len;
                    unsigned char* buf = charstrings_index->values[unicode[i]]->val.string;
                    _cff_do_render_char(context, deque, buf, len);

                    plutovg_canvas_fill(context->canvas);
                    plutovg_canvas_restore(context->canvas); 
                    pdf_deque_free(deque);
                }
                //plutovg_surface_write_to_png(context->surface, "test.png");
            }
        }
#endif
    }
    else
    {
        float x, y;
        plutovg_canvas_get_current_point(context->canvas, &x, &y);
        // render Type3 font
        pdf_array_t* differences = context->state->textState.font->differences;
        if (differences != NULL)
        {
            pdf_array_t* font_bbox = context->state->textState.font->font_bbox;
            pdf_array_t* font_matrix = context->state->textState.font->font_matrix;
            pdf_array_t* widths = context->state->textState.font->widths;
            // plutovg_canvas_scale(context->canvas, 1, -1);
            
            //plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
            //plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
            plutovg_matrix_t original_matrix = context->state->textState.textMatrix;
            for (int i = 0; i < unicode_cnt; i++) 
            {
                uint16_t c = unicode[i];
                for (int j = 0; j < differences->num_elements; j += 2) 
                {
                    if ((int)differences->values[j]->val.number == c) 
                    {
                        const char* name = differences->values[j + 1]->val.name;
                        int ref = pdf_dict_get_ref(context->state->textState.font->charProcs, name);
                        if (ref != -1) 
                        {
                            pdf_obj_t* obj = pdf_file_get_obj(context->pdf, ref);
                            if (obj != NULL && obj->stream != NULL) 
                            {
                                pdf_obj_t* save_obj = context->current_obj;
                                context->current_obj = obj;

                                plutovg_canvas_save(context->canvas);
                                plutovg_matrix_t font_matrix_plutovg, rm;
                                plutovg_matrix_init(&font_matrix_plutovg, 
                                    font_matrix->values[0]->val.number, font_matrix->values[1]->val.number,
                                    font_matrix->values[2]->val.number, font_matrix->values[3]->val.number,
                                    font_matrix->values[4]->val.number, font_matrix->values[5]->val.number);
                                plutovg_matrix_multiply(&rm, &font_matrix_plutovg, &context->state->textState.textMatrix);
                                plutovg_matrix_t m = {
                                    context->state->textState.fontSize * context->state->textState.horizontalScaling / 100, 0,
                                    0, context->state->textState.fontSize,
                                    0, context->state->textState.textRise
                                };
                                plutovg_matrix_multiply(&rm, &m, &rm);
                                plutovg_canvas_transform(context->canvas, &rm);
                                if (widths != NULL)
                                {
                                    float width = 0;
                                    if (c >= context->state->textState.font->first_char && c <= context->state->textState.font->last_char)
                                    {
                                        width = widths->values[c - context->state->textState.font->first_char]->val.number;
                                    }
                                    // plutovg_canvas_translate(context->canvas, x + context->state->textState.textLineWidth, 0);
                                    double advance = width * context->state->textState.fontSize / 1000.0;
                                    context->state->textState.textLineWidth += advance;
                                    plutovg_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                                }
                                plutovg_canvas_set_rgb(context->canvas,
                                    context->state->fill.color[0],
                                    context->state->fill.color[1],
                                    context->state->fill.color[2]);
                                // if (font_bbox != NULL) 
                                // {
                                //     //plutovg_canvas_new_path(context->canvas);
                                //     plutovg_canvas_rect(context->canvas,
                                //         font_bbox->values[0]->val.number,
                                //         font_bbox->values[1]->val.number,
                                //         font_bbox->values[2]->val.number,
                                //         font_bbox->values[3]->val.number);
                                //     plutovg_canvas_clip(context->canvas);
                                // }
                                plutovg_canvas_new_path(context->canvas);
                                pdf_stream_open(obj->stream);
                                pdf_parser_token_t* tk = NULL;
                                while ((tk = pdf_stream_get_next_token(obj->stream)) != NULL) 
                                {
                                    _do_render_operation(context, tk);
                                    pdf_parser_token_free(obj->stream->parser, tk);
                                }
                                pdf_stream_close(obj->stream);
                                context->current_obj = save_obj;
                                
                                plutovg_canvas_restore(context->canvas);
                            }
                        }
                        break;
                    }
                }
            }
            context->state->textState.textMatrix = original_matrix;
        }
    }
    plutovg_canvas_restore(context->canvas);
}
