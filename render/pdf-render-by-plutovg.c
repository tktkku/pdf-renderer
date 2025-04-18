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
    pdf_deque_t* deque = pdf_deque_init();
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
    context.deque = deque;
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
    pdf_deque_free(deque);
    for (int i = 0; i < cvector_size(context.fontcache); i++)
    {
        pdf_font_cache_t* fontcache = context.fontcache[i];
        if (fontcache->font != NULL)
        {
            pdf_font_free(fontcache->font);
        }
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
        // push data to deque
        pdf_deque_push(context->deque, tk->token, tk->token_len);
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
                    if (!strcmp(tk->token, "Tj") || !strcmp(tk->token, "TJ"))
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

void handle_BDC(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    if (!strcmp(buf, ">>"))
    {
        pdf_deque_pop_front(context->deque, &node); // str
        pdf_deque_pop_front(context->deque, &node); // name
        pdf_deque_pop_front(context->deque, &node); // <<
    }
    else
    {
        pdf_deque_pop_front(context->deque, &node); // name
    }
    pdf_deque_pop_front(context->deque, &node); // tag
    
}
void handle_BI(pdf_context_t* context)
{
    // begin an inline image object
}

void handle_BMC(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}


void handle_cm(pdf_context_t* context)
{
    // change matrix CTM
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float f = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float e = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float d = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float c = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float b = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float a = strtof(buf, NULL);

    plutovg_matrix_t ctm;
    plutovg_matrix_init(&ctm, a, b, c, d, e, f);
    plutovg_canvas_transform(context->canvas, &ctm);
}



void handle_d(pdf_context_t* context)
{
    // set line dash pattern
    // dashArray dashPhase
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float offset = strtof(buf, NULL);

    int index = 2;
    float dashs[2] = { 0 };
    while (true)
    {
        pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float wy = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float wx = strtof(buf, NULL);
    // plutovg_canvas_translate(context->canvas, wx, wy);
}

void handle_d1(pdf_context_t* context)
{
    // wx wy llx lly urx ury
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float ury = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float urx = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float lly = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float llx = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float wy = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float wx = strtof(buf, NULL);
    // plutovg_canvas_translate(context->canvas, wx, wy);
    // plutovg_canvas_rect(context->canvas, llx, lly, urx - llx, ury - lly);
}

void handle_DP(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
}
void handle_EI(pdf_context_t* context)
{
    // end an inline image object
}

void handle_EMC(pdf_context_t* context) {}

void handle_gs(pdf_context_t* context)
{
    // set specified parameters
    // dictName shall be the name of
    // a graphics state parameter dictionary
    // in the ExtGState subdictionary of the current resource dictionary
    // dictName
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    // pdf_page_get_ext_gstate(context->page, buf);
}

void handle_i(pdf_context_t* context)
{
    // set flatness tolerance
    // flatness
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    int j = atoi(buf);

    plutovg_canvas_set_line_join(context->canvas, j);
}

void handle_J(pdf_context_t* context)
{
    // set cap style
    // lineCap
    pdf_node_t node;
    char buf[1024] = { 0 };
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    int c = atoi(buf);

    plutovg_canvas_set_line_cap(context->canvas, c);
}


void handle_M(pdf_context_t* context)
{
    // set miter limit
    // miterLimit
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float m = strtof(buf, NULL);
    plutovg_canvas_set_miter_limit(context->canvas, m);
}

void handle_MP(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
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


void handle_ri(pdf_context_t* context)
{
    // set color rendering intent
    // intent
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}

void handle_sh(pdf_context_t* context)
{
    // name
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}