#include "render.h"
#include "pdf-private.h"
#include <plutovg-private.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void render_to_png_by_plutovg(pdf_page_t* page, char* filename)
{
    if (page == NULL || filename == NULL)
    {
        return;
    }
    int width = pdf_page_get_media_width(page) * PIXELS_PER_POINT;
    int height = pdf_page_get_media_height(page) * PIXELS_PER_POINT;
    int stride = width * 4;
    unsigned char* pixels = (unsigned char*)malloc(stride * height);
    memset(pixels, 0xFF, stride * height);
    render_to_buffer_by_plutovg(page, pixels, width, height, stride);
    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels, width, height, stride);
    plutovg_surface_write_to_png(surface, filename);
    plutovg_surface_destroy(surface);
    free(pixels);
}

void render_to_buffer_by_plutovg(pdf_page_t* page, unsigned char* pixels,
    int width, int height, int stride)
{
    pdf_stack_t* stack = pdf_stack_init();
    FT_Library ft_library = NULL;
    FT_Init_FreeType(&ft_library);
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
    context.stack = stack;
    context.pdf = page->pdf;
    context.page = page;

    context.state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memset(context.state, 0, sizeof(pdf_graphics_state_t));
    strcpy(context.state->fill.currentColorSpace, "DeviceGray");
    strcpy(context.state->stroke.currentColorSpace, "DeviceGray");
    context.state->stroke.color[0] = 0;
    context.state->stroke.color[1] = 0;
    context.state->stroke.color[2] = 0;
    context.state->fill.color[0] = 0;
    context.state->fill.color[1] = 0;
    context.state->fill.color[2] = 0;
    
    context.state->lineWidth = 1.0;
    context.state->lineCap = 0;
    context.state->lineJoin = 0;
    context.state->miterLimit = 10.0;

    context.state->textState.characterSpacing = 0;
    context.state->textState.wordSpacing = 0;
    context.state->textState.horizontalScaling = 100;
    context.state->textState.textLeading = 0;
    context.state->textState.textMode = 0;
    context.state->textState.textRise = 0;
    
    context.state->textState.font = NULL;
    context.state->textState.fontface = NULL;
    context.current_obj = page->obj;
    context.fontcache = NULL;
    context.surface = surface;
    context.ft_library = ft_library;
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
            _do_render_operation(&context, tk);
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
    for (int i = 0; i < cvector_size(context.fontcache); i++)
    {
        pdf_font_cache_t* fontcache = context.fontcache[i];
        // if (fontcache->font != NULL)
        // {
        //     pdf_font_free(fontcache->font);
        // }
        if (fontcache->fontface != NULL)
        {
            plutovg_font_face_destroy(fontcache->fontface);
        }
        if (fontcache->ft_face != NULL)
        {
            FT_Done_Face(fontcache->ft_face);
        }
        free(fontcache);
    }
    cvector_free(context.fontcache);
    free(context.state);
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
    FT_Done_FreeType(ft_library);
}
#define DEBUG 0
void _do_render_operation(pdf_context_t* context, pdf_parser_token_t* tk)
{
#if DEBUG
    printf("%s\n", tk->token);
#endif
    if (tk->type != TOKEN_OPERATOR)
    {
        // did not match any operation
        // push data to stack
        pdf_stack_push(context->stack, tk->token, tk->token_len);
    }
    else
    {
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
#if DEBUG
                    if (!strcmp(tk->token, "f"))
                    {
                        plutovg_surface_write_to_png(context->surface, "test.png");
                        printf("Press any key to continue...");
                        getchar();
                    }
#endif
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
    }
}
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
                } else if (tag & FT_CURVE_TAG_CUBIC) {
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
            if (context->state->textState.fontface != NULL)
            {
                context->state->textState.textLineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0);
            }
            else
            {
                context->state->textState.textLineWidth += _canvas_fill_text(context, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->state->textState.textLineWidth, 0, true); 
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

void handle_apostrophe(pdf_context_t* context)
{
    // move to the next line and show a text string
    // string
    // same as
    // T*
    // string Tj
    context->state->textState.textLineWidth = 0;
    handle_T_star(context);
    handle_Tj(context);
}

void handle_b_star(pdf_context_t* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B_star(pdf_context_t* context)
{
    // fill and then stroke the path, using the even-odd rule
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_b(pdf_context_t* context)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B(pdf_context_t* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}
void handle_BDC(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    if (!strcmp(buf, ">>"))
    {
        pdf_stack_pop(context->stack, &node); // str
        pdf_stack_pop(context->stack, &node); // name
        pdf_stack_pop(context->stack, &node); // <<
    }
    else
    {
        pdf_stack_pop(context->stack, &node); // name
    }
    pdf_stack_pop(context->stack, &node); // tag
    
}
void handle_BI(pdf_context_t* context)
{
    // begin an inline image object
}

void handle_BMC(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}
void handle_BT(pdf_context_t* context)
{
    // begin text
    plutovg_canvas_save(context->canvas);
    //plutovg_canvas_move_to(context->canvas, 0, 0);
    // context->fontface = NULL;
    // context->font = NULL;
    context->state->textState.textLineWidth = 0;
    plutovg_matrix_init_identity(&context->state->textState.textMatrix);
}

void handle_c(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y1 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}

void handle_cm(pdf_context_t* context)
{
    // change matrix CTM
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float a = strtof(buf, NULL);

    plutovg_matrix_t ctm;
    plutovg_matrix_init(&ctm, a, b, c, d, e, f);
    plutovg_canvas_transform(context->canvas, &ctm);
}

void handle_cs(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // stack_node_t node;
    // node.data = buf;
    // stack_pop(context->stack, &node);

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    if (!strcmp(buf, "/DeviceGray") || !strcmp(buf, "/DeviceRGB") || !strcmp(buf, "/DeviceCMYK"))
    {
        strcpy(context->state->fill.currentColorSpace, buf);
    }
    else
    {
        pdf_obj_get_colorspace(context->current_obj, buf, context->state->fill.currentColorSpace);
    }
}

void handle_CS(pdf_context_t* context)
{
    // set color space
    // /DeviceGray
    // initialize the corresponding current color to 0.0
    /// DeviceRGB
    // initialize the corresponding current color of red green blue to 0.0
    // /DeviceCMYK
    // initialize the corresponding current color of cyan magenta yellow to 0.0
    // and the black to 1.0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    if (!strcmp(buf, "/DeviceGray") || !strcmp(buf, "/DeviceRGB") || !strcmp(buf, "/DeviceCMYK"))
    {
        strcpy(context->state->stroke.currentColorSpace, buf);
    }
    else
    {
        pdf_obj_get_colorspace(context->current_obj, buf, context->state->stroke.currentColorSpace);
    }
}

void handle_d(pdf_context_t* context)
{
    // set line dash pattern
    // dashArray dashPhase
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float offset = strtof(buf, NULL);

    int index = 2;
    float dashs[2] = { 0 };
    while (true)
    {
        pdf_stack_pop(context->stack, &node);
        if (!strcmp(node.data, "]"))
        {
            // ignore
        }
        else if (!strcmp(node.data, "["))
        {
            break;
        }
        else
        {
            float dash = strtof(buf, NULL);
            if (index > 0)
                dashs[--index] = dash;
        }
    }

    plutovg_canvas_set_dash_offset(context->canvas, offset);
    plutovg_canvas_set_dash_array(context->canvas, dashs, 2);
}

void handle_d0(pdf_context_t* context)
{
    // wx wy
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float wy = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float wx = strtof(buf, NULL);
    // plutovg_canvas_translate(context->canvas, wx, wy);
}

void handle_d1(pdf_context_t* context)
{
    // wx wy llx lly urx ury
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ury = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float urx = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float lly = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float llx = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float wy = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float wx = strtof(buf, NULL);
    // plutovg_canvas_translate(context->canvas, wx, wy);
    // plutovg_canvas_rect(context->canvas, llx, lly, urx - llx, ury - lly);
}

void handle_DP(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}
void handle_EI(pdf_context_t* context)
{
    // end an inline image object
}

void handle_EMC(pdf_context_t* context) {}
void handle_ET(pdf_context_t* context)
{
    // end text
    plutovg_canvas_restore(context->canvas);
    // if (context->fontface)
    // {
    //     plutovg_canvas_set_font_face(context->canvas, NULL);
    //     plutovg_font_face_destroy(context->fontface);
    // }
    // if (context->font)
    // {
    //     pdf_font_free(context->font);
    //     context->font = NULL;
    // }
}

void handle_F_f(pdf_context_t* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_f_star(pdf_context_t* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_g(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    // handle_G(context);
    //plutovg_canvas_set_rgb(context->canvas, g, g, g);
    context->state->fill.color[0] = g;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = g;
    strcpy(context->state->fill.currentColorSpace, "/DeviceGray");
}

void handle_G(pdf_context_t* context)
{
    // set both in one operation
    // gray
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    context->state->stroke.color[0] = g;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = g;
    strcpy(context->state->stroke.currentColorSpace, "/DeviceGray");
}

void handle_gs(pdf_context_t* context)
{
    // set specified parameters
    // dictName shall be the name of
    // a graphics state parameter dictionary
    // in the ExtGState subdictionary of the current resource dictionary
    // dictName
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    // pdf_page_get_ext_gstate(context->page, buf);
}

void handle_h(pdf_context_t* context)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    plutovg_canvas_close_path(context->canvas);
}

void handle_i(pdf_context_t* context)
{
    // set flatness tolerance
    // flatness
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    //float i = strtof(buf, NULL);
    // context->graphics_state.flatness = i;
}

void handle_ID(pdf_context_t* context)
{
    // begin the image data for an inline image object
}

void handle_j(pdf_context_t* context)
{
    // set join style
    // lineJoin

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int j = atoi(buf);

    plutovg_canvas_set_line_join(context->canvas, j);
}

void handle_J(pdf_context_t* context)
{
    // set cap style
    // lineCap
    pdf_stack_node_t node;
    char buf[1024] = { 0 };
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int c = atoi(buf);

    plutovg_canvas_set_line_cap(context->canvas, c);
}

void handle_k(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float k = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    // handle_K(context);
    float r = (1.0 - c) * (1.0 - k);
    float g = (1.0 - m) * (1.0 - k);
    float b = (1.0 - y) * (1.0 - k);
    //plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fill.color[0] = r;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = b;
    strcpy(context->state->fill.currentColorSpace, "/DeviceCMYK");
}

void handle_K(pdf_context_t* context)
{
    // combine CS and SC for DeviceCMYK
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float k = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->state->stroke.color[0] = (1.0 - c) * (1.0 - k);
    context->state->stroke.color[1] = (1.0 - m) * (1.0 - k);
    context->state->stroke.color[2] = (1.0 - y) * (1.0 - k);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceCMYK");
}

void handle_l(pdf_context_t* context)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_line_to(context->canvas, x, y);
}

void handle_m(pdf_context_t* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_move_to(context->canvas, x, y);
}

void handle_M(pdf_context_t* context)
{
    // set miter limit
    // miterLimit
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    plutovg_canvas_set_miter_limit(context->canvas, m);
}

void handle_MP(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}
void handle_n(pdf_context_t* context)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_new_path(context->canvas);
}

void handle_q(pdf_context_t* context)
{
    // store state
    plutovg_canvas_save(context->canvas);
    pdf_graphics_state_t* new_state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memcpy(new_state, context->state, sizeof(pdf_graphics_state_t));
    new_state->next = context->state;
    context->state = new_state;
}

void handle_Q(pdf_context_t* context)
{
    // restore state
    plutovg_canvas_restore(context->canvas);
    pdf_graphics_state_t* old_state = context->state;
    context->state = old_state->next;
    free(old_state);
    plutovg_canvas_set_line_width(context->canvas, context->state->lineWidth);
    plutovg_canvas_set_line_cap(context->canvas, (plutovg_line_cap_t)context->state->lineCap);
    plutovg_canvas_set_line_join(context->canvas, (plutovg_line_join_t)context->state->lineJoin);
    plutovg_canvas_set_miter_limit(context->canvas, context->state->miterLimit);
}

void handle_quotation(pdf_context_t* context)
{
    // move to the next line and show a text string
    // aw as the word spacing
    // ac as the character spacing
    // aw ac string
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    context->state->textState.textLineWidth = 0;
    // TODO
}

void handle_re(pdf_context_t* context)
{
    // append a rectangle to the current path as a complete subpath
    // lower-left corner (x, y)
    // x y width height

    /*
    the operation
    x y width height re
    is equivalent to
    x y m
    (x+width) y l
    (x+width) (y+height) l
    x (y+height) l
    h
    */

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float height = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float width = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);

    plutovg_canvas_rect(context->canvas, x, y, width, height);
}

void handle_rg(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);
    //plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fill.color[0] = r;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = b;
    strcpy(context->state->fill.currentColorSpace, "/DeviceRGB");
    // handle_RG(context);
}

void handle_RG(pdf_context_t* context)
{
    // combine CS and SC for DeviceRGB
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);
    context->state->stroke.color[0] = r;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = b;
    // plutovg_canvas_set_rgb(context->canvas, gray, gray, gray);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceRGB");
}

void handle_ri(pdf_context_t* context)
{
    // set color rendering intent
    // intent
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}

void handle_s(pdf_context_t* context)
{
    // close and stroke the path
    // same as the sequence h S
    //
    plutovg_canvas_close_path(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_S(pdf_context_t* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_sc(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // stack_node_t node;
    // node.data = buf;
    // stack_pop(context->stack, &node);
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;

    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->state->fill.color[0] = g;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = g;
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        pdf_stack_pop(context->stack, &node);
        float b = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float r = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, r, g, b);
        context->state->fill.color[0] = r;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = b;
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
        pdf_stack_pop(context->stack, &node);
        float k = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float y = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float m = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float c = strtof(buf, NULL);

        float r = (1.0 - c) * (1.0 - k);
        float g = (1.0 - m) * (1.0 - k);
        float b = (1.0 - y) * (1.0 - k);

        context->state->fill.color[0] = r;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = b;
    }
}

void handle_SC(pdf_context_t* context)
{
    // set gray level, 0.0 to balck 1.0 to white

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;

    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->state->stroke.color[0] = g;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = g;
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        pdf_stack_pop(context->stack, &node);
        float b = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float r = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, r, g, b);
        context->state->stroke.color[0] = r;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = b;
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
        pdf_stack_pop(context->stack, &node);
        float k = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float y = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float m = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float c = strtof(buf, NULL);

        float r = (1.0 - c) * (1.0 - k);
        float g = (1.0 - m) * (1.0 - k);
        float b = (1.0 - y) * (1.0 - k);

        context->state->stroke.color[0] = r;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = b;
    }
}

void handle_scn(pdf_context_t* context)
{
    // char buf[1024] = { 0 };
    // pdf_stack_node_t node;
    // node.data = buf;
    // pdf_stack_pop(context->stack, &node);
    // pdf_stack_pop(context->stack, &node);
    // pdf_stack_pop(context->stack, &node);
    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_sc(context);
    }
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_SCN(pdf_context_t* context)
{
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_SC(context);
    }
  
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_sh(pdf_context_t* context)
{
    // name
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}

void stroke(pdf_context_t* context)
{
    //double r, g, b, a;
    plutovg_color_t color;
    plutovg_canvas_get_color(context->canvas, &color);
    plutovg_canvas_set_rgb(context->canvas, context->state->stroke.color[0],
        context->state->stroke.color[1], context->state->stroke.color[2]);
    plutovg_canvas_stroke(context->canvas);
    plutovg_canvas_set_color(context->canvas, &color);
    // plutovg_canvas_set_rgb(context->canvas,
    //     context->fillColor[0],
    //     context->fillColor[1],
    //     context->fillColor[2]);
}

void handle_T_star(pdf_context_t* context)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    // plutovg_canvas_translate(context->canvas, 0, -context->state->textState.textLeading);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
    context->state->textState.textLineWidth = 0;
}

void handle_Tc(pdf_context_t* context)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->state->textState.characterSpacing = c;
}

void handle_Td(pdf_context_t* context)
{
    // set start position on the page
    // tx ty
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ty = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float tx = strtof(buf, NULL);

    // plutovg_canvas_translate(context->canvas, tx, ty);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
    context->state->textState.textLineWidth = 0;
}

void handle_TD(pdf_context_t* context)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ty = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float tx = strtof(buf, NULL);

    // plutovg_canvas_translate(context->canvas, tx, ty);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
    context->state->textState.textLeading = -ty;
    context->state->textState.textLineWidth = 0;
    // side effect, set the leading parameter in the text state
    // -ty TL
    // tx ty Td
}

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

    _do_text_render(context, node.data, node.size);
}
void handle_TJ(pdf_context_t* context)
{
    // show one or more text strings
    // array
    // if the element is a string , show the string
    // if the element is a number, adjust the position
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_t* tmp_stack = pdf_stack_init();
    while (true)
    {
        pdf_stack_pop(context->stack, &node);
        if (!strcmp(buf, "]"))
        {
            // ignore
        }
        else if (!strcmp(buf, "["))
        {
            break;
        }
        else
        {
            pdf_stack_push(tmp_stack, buf, strlen(buf));
        }
    }
    if (context->state->textState.font == NULL)
    {
        pdf_stack_free(tmp_stack);
        return;
    }
    memset(buf, 0, sizeof(buf));
    while (tmp_stack->top != NULL)
    {
        pdf_stack_pop(tmp_stack, &node);
        if (buf[0] == '<' || buf[0] == '(')
        {
            _do_text_render(context, node.data, node.size);
        }
        else // a number
        {
            float a = strtof(buf, NULL);
            //plutovg_canvas_translate(context->canvas, -a, 0);
            //plutovg_canvas_move_to(context->canvas, 0, 0);
            context->state->textState.textLineWidth -= (a * (context->state->textState.fontSize / 1000.0));
        }
    }

    pdf_stack_free(tmp_stack);
}

void handle_TL(pdf_context_t* context)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float t = strtof(buf, NULL);
    context->state->textState.textLeading = t;
}

void handle_Tm(pdf_context_t* context)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float a, b, c, d, e, f;
    pdf_stack_pop(context->stack, &node);
    f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    a = strtof(buf, NULL);

    plutovg_matrix_t m;
    plutovg_matrix_init(&m, a, b, c, d, e, f);

    //plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
    // set font matrix
    // plutovg_canvas_transform(context->canvas, &m);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    context->state->textState.textMatrix = m;
}

void handle_Tr(pdf_context_t* context)
{
    // set text rendering mode
    // mode initial value =0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int v = strtof(buf, NULL);
    // STROKE FILL BOTH CLIP

    int mode = 0;
    if ((v & 0x1) == 0)
    {
        mode |= 2;
    }
    if ((v & 0x4) != 0)
    {
        mode |= 4;
    }
    if (((v & 0x1) ^ ((v & 0x2) >> 1)) != 0)
    {
        mode |= 1;
    }
    context->state->textState.textMode = mode;
}

void handle_Ts(pdf_context_t* context)
{
    // set text rise
    // rise initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);

    context->state->textState.textRise = r;
}

void handle_Tw(pdf_context_t* context)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    context->state->textState.wordSpacing = w;
}

void handle_Tz(pdf_context_t* context)
{
    // horizontal scaling
    // scale initial value=100
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float h = strtof(buf, NULL);
    //plutovg_canvas_scale(context->canvas, h / 100.0, 1.0);
    context->state->textState.horizontalScaling = h;
}

void handle_v(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float x1, y1, x2, y2, x3, y3;
    pdf_stack_pop(context->stack, &node);
    y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x2 = strtof(buf, NULL);

    plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}
void handle_W_star(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule

    plutovg_canvas_clip(context->canvas);
}

void handle_w(pdf_context_t* context)
{
    // set line width
    // lineWidth
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    plutovg_canvas_set_line_width(context->canvas, w);
}

void handle_W(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule

    plutovg_canvas_clip(context->canvas);
}

void handle_y(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current path
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x3, y3) as the Bezier control points
    // x2 y2 same as x3 y3
    // x1 y1 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y1 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x3, y3, x3, y3);
}
