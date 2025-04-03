#include "render.h"
void render_to_png_by_plutovg(pdf_page_t* page, char* filename)
{
    int width = pdf_page_get_media_width(page) * PIXELS_PER_POINT;
    int height = pdf_page_get_media_height(page) * PIXELS_PER_POINT;
    int stride = width * 4;
    unsigned char* pixels = (unsigned char*)malloc(stride * height);
    memset(pixels, 0xFF, stride * height);
    pdf_stack_t* stack = pdf_stack_init();

    pdf_context_t context;

    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels, width, height, stride);
    // plutovg_surface_t* surface = plutovg_surface_create(width, height);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 0.5);
    plutovg_canvas_set_miter_limit(canvas, 10.0);

    plutovg_canvas_restore(canvas);

    // Flip the Y-axis
    plutovg_canvas_translate(canvas, 0, height);
    plutovg_canvas_scale(canvas, PIXELS_PER_POINT, -PIXELS_PER_POINT);
    context.canvas = canvas;
    context.state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memset(context.state, 0, sizeof(pdf_graphics_state_t));
    strcpy(context.state->currentColorSpace, "DeviceGray");
    context.state->strokeColor[0] = 0;
    context.state->strokeColor[1] = 0;
    context.state->strokeColor[2] = 0;
    context.state->fillColor[0] = 0;
    context.state->fillColor[1] = 0;
    context.state->fillColor[2] = 0;
    context.stack = stack;
    context.pdf = page->pdf;
    context.page = page;
    context.state->lineWidth = 1.0;
    context.state->lineCap = 0;
    context.state->lineJoin = 0;
    context.state->miterLimit = 10.0;
    context.state->textState.textMode = 0;
    context.state->textState.textLeading = 0;
    context.state->textState.font = NULL;
    context.state->textState.fontface = NULL;
    context.current_obj = NULL;
    context.fontcache = NULL;
    int numStreams = pdf_page_get_streams(page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(page, j);
        if (stream == NULL)
            continue;
        pdf_stream_open(stream);
        pdf_parser_token_t* tk;
        //int count = 0;
        while ((tk = pdf_stream_get_next_token(stream)) != NULL)
        {
            const char* token = pdf_parser_token_get_token(tk);
            if (token == NULL)
                continue;
            // printf("%s\n", token);
            _do_render_operation(&context, tk);
            // if (!strcmp(token, "528.1"))
            // {
            //     plutovg_surface_write_to_png(surface, "test.png");
            //     printf("Press any key to continue...");
            //     getchar();
            // }
            pdf_parser_token_free(stream->parser, tk);
        }

        pdf_stream_close(stream);
    }
    // Annots
    if (page->annots != NULL)
    {
        for (int i = 0; i < page->annots->num_elements; i++)
        {
            pdf_obj_t* anno_obj = pdf_file_get_obj(page->pdf, page->annots->values[i]->val.indirect);
            pdf_dict_get_name(anno_obj->value->val.dict, "/Type");
            pdf_dict_get_name(anno_obj->value->val.dict, "/SubType");
            pdf_array_t* rect_aar = pdf_dict_get_array(anno_obj->value->val.dict, "/Rect");
            if (rect_aar != NULL)
            {
                plutovg_canvas_translate(canvas, rect_aar->values[0]->val.number, rect_aar->values[1]->val.number);
            }
            pdf_dict_get_string(anno_obj->value->val.dict, "/Contents");
            pdf_dict_get_dict(anno_obj->value->val.dict, "/P");
            pdf_dict_get_string(anno_obj->value->val.dict, "/NM");
            pdf_dict_get_string(anno_obj->value->val.dict, "/M");
            int F = pdf_dict_get_number(anno_obj->value->val.dict, "/F");
            if (F != -1)
            {
                if (F & 0b0000000001); // invisible
                if (F & 0b0000000010); // hidden
                if (F & 0b0000000100); // print
                if (F & 0b0000001000); // nozoom
                if (F & 0b0000010000); // norotate
                if (F & 0b0000100000); // noview
                if (F & 0b0001000000); // readonly
                if (F & 0b0010000000); // locked
                if (F & 0b0100000000); // togglenoview
                if (F & 0b1000000000); // lockedcontents
            }
            pdf_dict_t* AP = pdf_dict_get_dict(anno_obj->value->val.dict, "/AP");
            if (AP != NULL)
            {
                pdf_dict_t* nomal_dict = pdf_dict_get_dict(AP, "/N"); // required
                if (nomal_dict == NULL)
                {
                    int ref = pdf_dict_get_ref(AP, "/N");
                    pdf_obj_t* obj = pdf_file_get_obj(page->pdf, ref);
                    if (obj->stream != NULL)
                    {
                        context.current_obj = obj;
                        pdf_stream_open(obj->stream);
                        pdf_parser_token_t* tk = NULL;
                        while ((tk = pdf_stream_get_next_token(obj->stream)) != NULL)
                        {
                            _do_render_operation(&context, tk);
                            pdf_parser_token_free(obj->stream->parser, tk);
                        }
                        pdf_stream_close(obj->stream);
                    }
                }
            }
            pdf_dict_get_name(anno_obj->value->val.dict, "/AS");
            pdf_dict_get_array(anno_obj->value->val.dict, "/Border");
            pdf_dict_get_array(anno_obj->value->val.dict, "/C");
            pdf_dict_get_number(anno_obj->value->val.dict, "/StructParent");
            pdf_dict_get_dict(anno_obj->value->val.dict, "/OC");
        }
    }
    pdf_stack_free(stack);
    plutovg_surface_write_to_png(surface, filename);
    int nums = cvector_size(context.fontcache);
    for (int i = 0; i < nums; i++) 
    {
        pdf_font_free(context.fontcache[i]->font);
        plutovg_canvas_set_font_face(context.canvas, NULL);
        plutovg_font_face_destroy(context.fontcache[i]->fontface);
        free(context.fontcache[i]);
    }
    cvector_free(context.fontcache);
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
    free(pixels);
}

void render_to_buffer_by_plutovg(pdf_page_t* page, unsigned char* pixels,
    int width, int height, int stride)
{
    pdf_stack_t* stack = pdf_stack_init();

    pdf_context_t context;

    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels, width, height, stride);
    // plutovg_surface_t* surface = plutovg_surface_create(width, height);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 0.5);
    plutovg_canvas_set_miter_limit(canvas, 10.0);

    plutovg_canvas_restore(canvas);

    // Flip the Y-axis
    plutovg_canvas_translate(canvas, 0, height);
    plutovg_canvas_scale(canvas, PIXELS_PER_POINT, -PIXELS_PER_POINT);
    context.canvas = canvas;
    context.state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memset(context.state, 0, sizeof(pdf_graphics_state_t));
    strcpy(context.state->currentColorSpace, "DeviceGray");
    context.state->strokeColor[0] = 0;
    context.state->strokeColor[1] = 0;
    context.state->strokeColor[2] = 0;
    context.state->fillColor[0] = 0;
    context.state->fillColor[1] = 0;
    context.state->fillColor[2] = 0;
    context.stack = stack;
    context.current_obj = NULL;
    context.pdf = page->pdf;
    context.page = page;
    context.state->lineWidth = 1.0;
    context.state->lineCap = 0;
    context.state->lineJoin = 0;
    context.state->miterLimit = 10.0;
    context.state->textState.textMode = 0;
    context.state->textState.textLeading = 0;
    context.state->textState.font = NULL;
    context.state->textState.fontface = NULL;
    context.current_obj = NULL;
    int numStreams = pdf_page_get_streams(page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(page, j);
        if (stream == NULL)
            continue;
        pdf_stream_open(stream);
        pdf_parser_token_t* tk;
        //int count = 0;
        while ((tk = pdf_stream_get_next_token(stream)) != NULL)
        {
            const char* token = pdf_parser_token_get_token(tk);
            if (token == NULL)
                continue;
            // printf("%s\n", token);
            _do_render_operation(&context, tk);
            // if (strcmp(token, "f") == 0)
            // {
            //     plutovg_surface_write_to_png(surface, "test.png");
            //     printf("Press any key to continue...");
            //     getchar();
            // }
            pdf_parser_token_free(stream->parser, tk);
        }

        pdf_stream_close(stream);
    }
    pdf_stack_free(stack);
    if (context.state->textState.fontface != NULL)
    {
        plutovg_canvas_set_font_face(context.canvas, NULL);
        plutovg_font_face_destroy(context.state->textState.fontface);
    }
    if (context.state->textState.font != NULL)
    {
        pdf_font_free(context.state->textState.font);
    }
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
}

void _do_render_operation(pdf_context_t* context, pdf_parser_token_t* tk)
{
    //printf("%s\n", tk->token);
    int count = ARRAY_COUNT(handlers);
    int left = 0;
    int right = count - 1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(handlers[mid].operation, tk->token);

        if (cmp == 0)
        {
            if (handlers[mid].handler != NULL)
            {
                if (strchr("fFbBW", tk->token[0]) != NULL)
                {
                    if (tk->token[1] == '\0')
                        plutovg_canvas_set_fill_rule(context->canvas,
                            PLUTOVG_FILL_RULE_NON_ZERO);
                    else if (tk->token[1] == '*')
                        plutovg_canvas_set_fill_rule(context->canvas,
                            PLUTOVG_FILL_RULE_EVEN_ODD);
                }
                handlers[mid].handler(context);
                return;
            }
        }
        else if (cmp < 0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    // did not match any operation
    // push data to stack
    pdf_stack_push(context->stack, tk->token, tk->token_len);
}

void _do_text_render(pdf_context_t* context, char* buf, int len)
{
    float x, y;
    plutovg_canvas_get_current_point(context->canvas, &x, &y);

    int unicode_cnt = 0;
    uint16_t unicode[1024] = { 0 };
    if (buf[0] == '<')
    {
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
    }
    else if (buf[0] == '(')
    {
        if (!strcmp(context->state->textState.font->subtype, "/TrueType"))
        {
            for (int i = 1; i < len; i++)
            {
                unicode[unicode_cnt++] = buf[i];
            }
        }
        else
        {
            if (strstr(context->state->textState.font->encoding, "Identity"))
            {
                for (int i = 1; i < len; i += 2)
                {
                    unicode[unicode_cnt++] = ((buf[i] << 8) & 0xFF00) | (buf[i + 1] & 0x00FF);
                }
            }
            else
            {
                for (int i = 1; i < len; i += 2)
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
}