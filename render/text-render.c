#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
#include "plutovg-private.h"
uint32_t _get_unicode_from_cmap(pdf_cmap_t* cmap, uint32_t code)
{
    bool found = false;
    while (cmap != NULL && !found)
    {
        for (int k = 0; k < cmap->unicode_map_len; k++)
        {
            if (code == cmap->unicode_map[k].cid)
            {
                return cmap->unicode_map[k].unicode;
            }
        }
 
        for (int k = 0; k < cmap->char_range_map_len && !found; k++)
        {
            if (code >= cmap->char_range_map[k].dstStart &&
                code <= cmap->char_range_map[k].srcEnd)
            {
                return cmap->char_range_map[k].dstStart +
                    (code - cmap->char_range_map[k].srcStart);
            }
        }
        cmap = cmap->next;
    }
    return code;
}

void _cff_do_render_char(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    //plutovg_canvas_t* canvas = context->canvas;
    //pdf_array_t* charstrings_index = context->charstrings;
    //pdf_array_t* global_subr_index = context->global_subr;
    //uint16_t global_bias = context->global_bias;

    unsigned char* end = context->buf + context->len;

    // for (int i = 0; i < context->len; i++)
    // {
    //     printf("%d ", context->cur[i]);
    // }
    // printf("\n");

    pdf_node_t node;
    char data[16] = {0};
    node.data = data;
    //static double width = 0;
    while (context->cur < end)
    {
        double v = 0;
        uint8_t b0 = context->cur[0];
        if (b0 == 28)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            uint8_t b2 = context->cur[1];
            context->cur += 2;
            v = b1 << 8 | b2; 
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 32 && b0 <= 246)
        { 
            context->cur++;
            v = b0 - 139;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 247 && b0 <= 250)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = (b0 - 247) * 256 + b1 + 108;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 251 && b0 <= 254)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = -(b0 - 251) * 256 - b1 - 108;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 == 255)
        {
            context->cur++;
            int32_t raw_value = (context->cur[0] << 24) | (context->cur[1] << 16) | (context->cur[2] << 8) | context->cur[3];
            int32_t signed_value = (int32_t)raw_value;
            v = (float)signed_value / 65536.0;
            context->cur += 4;
            pdf_deque_push(deque, &v, sizeof(double));
        } 
        else if (b0 == 11) break;
        else if (b0 == 14)
        {
            if (deque->size > 0 && !context->havewidth)
            {
                pdf_deque_pop_end(deque, &node);
                context->width = *((double*)data);
                context->havewidth = true;
            }
            pdf_deque_empty(deque);
            // if (context->open)
            //     plutovg_canvas_close_path(context->canvas);
            context->open = false;
            break;
        }

        uint8_t operator1 = context->cur[0];
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
                context->cur++;
                if (!context->fisr_stack_clear)
                {
                    if (operator1 != 10 && operator1 != 29)
                        pdf_deque_empty(deque);
                }
                else
                {
                    if (operator1 == 1//hstem
                    || operator1 == 3//vstem
                    || operator1 == 4//vmoveto
                    || operator1 == 18//hstemhm
                    || operator1 == 19//hintmask
                    || operator1 == 20//cntrmask
                    || operator1 == 21//rmoveto
                    || operator1 == 22//hmoveto
                    || operator1 == 23//vstemhm
                    )
                    {
                        pdf_deque_empty(deque);
                        context->fisr_stack_clear = false;
                        break;
                    }
                }
                break;
            }
            case 11://return
            {
                return;
            }
            case 14://endcar
            {
                if (deque->size > 0 && !context->havewidth)
                {
                    pdf_deque_pop_end(deque, &node);
                    context->width = *((double*)data);
                    context->havewidth = true;
                }
                pdf_deque_empty(deque);
                // if (context->open)
                //     plutovg_canvas_close_path(context->canvas);
                context->open = false;
                return;
            }
            case 12:
            {
                uint8_t operator2 = context->cur[1];
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
                        context->cur += 2;
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
}
typedef struct unicode_text
{
    union {
        uint8_t utf8;
        uint16_t utf16;
        uint32_t utf32;
    };
    plutovg_text_encoding_t encoding;
} unicode_text_t;
void _do_text_render(pdf_render_t* context, char* buf, int len)
{
    plutovg_canvas_save(context->canvas);
    int unicode_cnt = 0;
    unicode_text_t unicode[1024] = { 0 };
    unsigned char* pbuf = (unsigned char*)buf;
    if (pbuf[0] == '<')
    {
        // if (context->state->textState.font->encoding && strstr(context->state->textState.font->encoding, "Identity"))
        // {
        //     for (char* p = &pbuf[1]; *p != '\0'; p += 4)
        //     {
        //         uint16_t t = _hex_str_to_16bit(p);
        //         unicode[unicode_cnt].utf16 = t;
        //         unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF16;
        //         unicode_cnt++;
        //     }
        // }
        // else 
        if (context->state->textState.font->encoding == NULL)
        {
            for (char* p = &pbuf[1]; *p != '\0'; p += 2)
            {
                uint8_t t = _hex_str_to_8bit(p);
                unicode[unicode_cnt].utf8 = t;
                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF8;
                unicode_cnt++;
            }
        }
        else if (context->state->textState.font->basefont && !strcmp(context->state->textState.font->basefont, "/SimSun"))
        {
            for (char* p = &pbuf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                unicode[unicode_cnt].utf16 = t;
                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF16;
                unicode_cnt++;
            }
        }
        else if (context->state->textState.font->cmap)
        {
            pdf_cmap_t* cmap = context->state->textState.font->cmap;
            unsigned char* p = pbuf + 1;
            unsigned char* end = pbuf + len;
            while (p < end)
            {
                bool found = false;
                for (int len = 8; len >= 2; len /= 2)
                {
                    if (p + len > end) continue;
                    uint32_t code = _hex_str_to_32bit(p, len);
                    for (int i = 0; i < cmap->code_range_map_len; i++)
                    {
                        if (cmap->code_range_map[i].byte_len != (len / 2)) continue;
                        if (code >= cmap->code_range_map[i].srcStart && code <= cmap->code_range_map[i].srcEnd)
                        {
                            if (len == 8)
                            {
                                unicode[unicode_cnt].utf32 = _get_unicode_from_cmap(cmap, code);
                                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF32;
                            }
                            else if (len == 4)
                            {
                                unicode[unicode_cnt].utf16 = (uint16_t)_get_unicode_from_cmap(cmap, code);
                                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF16;
                            }
                            else
                            {
                                unicode[unicode_cnt].utf8 = (uint8_t)_get_unicode_from_cmap(cmap, code);
                                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF8;
                            }
                            unicode_cnt++;
                            p += len;
                            found = true;
                            break;
                        }
                    }
                    if (found) break;
                }
                // uint16_t t = _hex_str_to_16bit(p);
                // unicode[unicode_cnt++] = _get_unicode_from_cmap(context->state->textState.font->cmap, t);
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
        if (context->state->textState.font->subtype 
            && (!strcmp(context->state->textState.font->subtype, "/TrueType") || !strcmp(context->state->textState.font->subtype, "/Type3")))
        {
            for (int i = 1; i < len; i++)
            {
                unicode[unicode_cnt].utf8 = pbuf[i];
                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF8;
                unicode_cnt++;
            }
        }
        else
        {
            if (strstr(context->state->textState.font->encoding, "Identity"))
            {
                for (int i = 1; i < len; i += 2)
                {
                    unicode[unicode_cnt].utf16 = ((pbuf[i] << 8) & 0xFF00) | (pbuf[i + 1] & 0x00FF);
                    unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF16;
                    unicode_cnt++;
                }
            }
            else
            {
                for (int i = 1; i < len; i += 2)
                {
                    uint16_t t = ((pbuf[i] << 8) & 0xFF00) | (pbuf[i + 1] & 0x00FF);
                    unicode[unicode_cnt].utf16 = _get_unicode_from_cmap(context->state->textState.font->cmap, t);
                    unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF16;
                    unicode_cnt++;
                }
            }
        }
    }
    if (unicode_cnt == 0)
    {
        plutovg_canvas_restore(context->canvas);
        return;
    }

    if (!context->state->textState.font->subtype || strcmp(context->state->textState.font->subtype, "/Type3"))
    {
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
            plutovg_canvas_set_line_width(context->canvas, context->state->lineWidth);
            plutovg_canvas_set_miter_limit(context->canvas, context->state->miterLimit);
            plutovg_canvas_set_line_cap(context->canvas, context->state->lineCap);
            plutovg_canvas_set_line_join(context->canvas, context->state->lineJoin);

            plutovg_canvas_new_path(context->canvas);
            float advance_width = 0;
            // TODO: bold text support
            // bool bold = context->state->textState.font->font_weight == 700 
            //             || (context->state->textState.textMode == 2 || context->state->textState.textMode == 6);
            //float half_bold_width = 0;
            if (context->state->textState.font_face_loaded 
                && 
                (
                    context->state->textState.font->encoding == NULL
                    || 
                    !strstr(context->state->textState.font->encoding, "Identity")
                )
            )
            {
                for (int i = 0; i < unicode_cnt; i++)
                {
                    if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF8)
                    {
                        advance_width = plutovg_canvas_add_text(context->canvas, &unicode[i].utf8, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    else if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF16)
                    {
                        advance_width = plutovg_canvas_add_text(context->canvas, &unicode[i].utf16, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    else
                    {
                        advance_width = plutovg_canvas_add_text(context->canvas, &unicode[i].utf32, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    context->state->textState.textLineWidth += advance_width;
                }
                
                
                // if (bold)
                // {
                //     plutovg_canvas_add_text(context->canvas, unicode, unicode_cnt, PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth - half_bold_width, -half_bold_width);
                //     plutovg_canvas_add_text(context->canvas, unicode, unicode_cnt, PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth + half_bold_width, half_bold_width);
                // }
            }
            else
            {
                for (int i = 0; i < unicode_cnt; i++)
                {
                    if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF8)
                    {
                        advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf8, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    else if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF16)
                    {
                        advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf16, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    else
                    {
                        advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf32, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0);
                    }
                    context->state->textState.textLineWidth += advance_width;
                }
                // if (bold)
                // {
                //     plutovg_canvas_add_text1(context->canvas, unicode, unicode_cnt, PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth - half_bold_width, -half_bold_width);
                //     plutovg_canvas_add_text1(context->canvas, unicode, unicode_cnt, PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth + half_bold_width, half_bold_width);
                // }
            }
            //context->state->textState.textLineWidth += advance_width;
            // TODO: text rendering mode support
            switch (context->state->textState.textMode)
            {
                case 0:// fill
                case 4:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->fill.color[0],
                    context->state->fill.color[1],
                    context->state->fill.color[2]);
                    plutovg_canvas_fill(context->canvas);
                    break;
                case 1: // stroke
                case 5:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->stroke.color[0],
                    context->state->stroke.color[1],
                    context->state->stroke.color[2]);
                    plutovg_canvas_stroke(context->canvas);
                    break;
                case 2: // fill and then stroke
                case 6:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->stroke.color[0],
                    context->state->stroke.color[1],
                    context->state->stroke.color[2]);
                    plutovg_canvas_fill(context->canvas);
                    // plutovg_canvas_set_rgb(context->canvas,
                    // context->state->stroke.color[0],
                    // context->state->stroke.color[1],
                    // context->state->stroke.color[2]);
                    // plutovg_canvas_stroke(context->canvas);
                    break;
                // case 3: // invisible
                //     break;
                // case 4:
                //     plutovg_canvas_set_rgb(context->canvas,
                //     context->state->fill.color[0],
                //     context->state->fill.color[1],
                //     context->state->fill.color[2]);
                //     plutovg_canvas_fill(context->canvas);
                //     plutovg_canvas_clip_preserve(context->canvas);
                //     break;
                // case 5:
                //     plutovg_canvas_set_rgb(context->canvas,
                //     context->state->stroke.color[0],
                //     context->state->stroke.color[1],
                //     context->state->stroke.color[2]);
                //     plutovg_canvas_stroke(context->canvas);
                //     plutovg_canvas_clip_preserve(context->canvas);
                //     break;
                // case 6:
                //     plutovg_canvas_fill_preserve(context->canvas);
                //     plutovg_canvas_stroke(context->canvas);
                //     plutovg_canvas_new_path(context->canvas);
                //     break;
                // case 7:
                //     plutovg_canvas_clip_preserve(context->canvas);
                default:
                    break;
            }            
        }
        else
        {
            pdf_array_t* charstrings_index = context->state->textState.font->charstrings;
            pdf_array_t* font_dict_aar = context->state->textState.font->font_dict_arr;
            pdf_array_t* font_dict_select = context->state->textState.font->font_dict_select_arr;
            if (charstrings_index != NULL)
            {
                pdf_array_t* font_matrix = context->state->textState.font->font_matrix;
                plutovg_matrix_t original_matrix = context->state->textState.textMatrix;
                for (int i = 0; i < unicode_cnt; i++)
                {
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
                    pdf_cff_char_render_t ctx;
                    ctx.buf = (unsigned char*)charstrings_index->values[unicode[i].utf32]->val.string;
                    ctx.cur = ctx.buf;
                    ctx.len = charstrings_index->values[unicode[i].utf32]->value_len;
                    ctx.canvas = context->canvas;
                    ctx.fontSize = context->state->textState.fontSize;
                    ctx.global_bias = context->state->textState.font->global_subr_bias;
                    ctx.global_subr = context->state->textState.font->global_subr;
                    ctx.open = false;
                    ctx.havewidth = false;
                    ctx.width = 0;
                    ctx.curX = 0;
                    ctx.curY = 0;
                    ctx.stems = 0;
                    ctx.stemshm = 0;
                    ctx.fisr_stack_clear = true;
                    _cff_do_render_char(&ctx, deque);
                    pdf_dict_t* font_dict = NULL;
                    double defaultWidthX = 0;
                    double nominalWidthX = 0;
                    if ((int)(font_dict_select->values[0]->val.number) == 0)
                    {
                        int fd = font_dict_select->values[unicode[i].utf32 + 1]->val.number;
                        font_dict = font_dict_aar->values[fd]->val.dict;
                    }
                    else
                    {
                        for (int j = 1; j < font_dict_select->num_elements; j += 3)
                        {
                            if (unicode[i].utf32 >= (int)(font_dict_select->values[j]->val.number)
                            && unicode[i].utf32 <= (int)(font_dict_select->values[j + 1]->val.number))
                            {
                                int fd = (int)(font_dict_select->values[j + 2]->val.number);
                                font_dict = font_dict_aar->values[fd]->val.dict;
                                break;
                            }
                        }
                    }
                    if (font_dict != NULL)
                    {
                        defaultWidthX = pdf_dict_get_number(font_dict, "defaultWidthX");
                        nominalWidthX = pdf_dict_get_number(font_dict, "nominalWidthX");
                    }
                    double advance = defaultWidthX;
                    if ((int)(ctx.width) != 0)
                    {
                        advance = (ctx.width + nominalWidthX) * context->state->textState.fontSize / 1000.0;
                    }

                    context->state->textState.textLineWidth += advance;
                    plutovg_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                    plutovg_canvas_fill(context->canvas);
                    plutovg_canvas_restore(context->canvas); 
                    pdf_deque_free(deque);
                }
                context->state->textState.textMatrix = original_matrix;
                //plutovg_surface_write_to_png(context->surface, "test.png");
            }
        }
    }
    else
    {
        float x, y;
        plutovg_canvas_get_current_point(context->canvas, &x, &y);
        // render Type3 font
        pdf_array_t* differences = context->state->textState.font->differences;
        if (differences != NULL)
        {
            // pdf_array_t* font_bbox = context->state->textState.font->font_bbox;
            pdf_array_t* font_matrix = context->state->textState.font->font_matrix;
            pdf_array_t* widths = context->state->textState.font->widths;
            // plutovg_canvas_scale(context->canvas, 1, -1);
            
            //plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
            //plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
            plutovg_matrix_t original_matrix = context->state->textState.textMatrix;
            for (int i = 0; i < unicode_cnt; i++) 
            {
                uint32_t c = unicode[i].utf32;
                for (int j = 0; j < differences->num_elements; j += 2) 
                {
                    if ((uint32_t)differences->values[j]->val.number == c) 
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
                                    if (tk->type == TOKEN_STREAM_END)
                                        break;
                                    _do_render_operation(obj->stream, context, tk);
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
