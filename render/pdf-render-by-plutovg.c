#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
#include <plutovg-private.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void _init_state(pdf_render_t* context)
{
    context->state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memset(context->state, 0, sizeof(pdf_graphics_state_t));
    strcpy(context->state->fill.currentColorSpace, "DeviceGray");
    strcpy(context->state->stroke.currentColorSpace, "DeviceGray");
    context->state->stroke.color[0] = 0;
    context->state->stroke.color[1] = 0;
    context->state->stroke.color[2] = 0;
    context->state->fill.color[0] = 0;
    context->state->fill.color[1] = 0;
    context->state->fill.color[2] = 0;
    
    context->state->lineWidth = 1.0;
    context->state->lineCap = 0;
    context->state->lineJoin = 0;
    context->state->miterLimit = 10.0;

    context->state->textState.characterSpacing = 0;
    context->state->textState.wordSpacing = 0;
    context->state->textState.horizontalScaling = 100;
    context->state->textState.textLeading = 0;
    context->state->textState.textMode = 0;
    context->state->textState.textRise = 0;
    
    context->state->textState.font = NULL;
    context->state->textState.fontface = NULL;
}
pdf_render_t* pdf_render_init_with_size(pdf_page_t* page, int width, int height, int stride, int dpi)
{
    if (page == NULL) return NULL;
    pdf_render_t* r = (pdf_render_t*)malloc(sizeof(pdf_render_t));
    memset(r, 0, sizeof(pdf_render_t));
    r->page = page;
    r->pdf = page->pdf;
    r->current_obj = page->obj;

    unsigned char* pixels = (unsigned char*)malloc(stride * height);
    memset(pixels, 0xFF, stride * height);
    r->pixels = pixels;
    r->width = width;
    r->height = height;
    r->stride = stride;

    pdf_deque_t* deque = pdf_deque_init();

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
    plutovg_canvas_scale(canvas, dpi / 72.0, -(dpi / 72.0));
    r->canvas = canvas;
    r->deque = deque;
    _init_state(r);
    
    r->fontcache = NULL;
    r->surface = surface;
    return r;
}
pdf_render_t* pdf_render_init(pdf_page_t* page, int dpi)
{
    if (page == NULL) return NULL;
    double dpi_scale = dpi / 72.0;
    int width = pdf_page_get_media_width(page) * dpi_scale;
    int height = pdf_page_get_media_height(page) * dpi_scale;
    int stride = width * 4;
    return pdf_render_init_with_size(page, width, height, stride, dpi);
}
pdf_render_t* pdf_render_init_for_paper(pdf_page_t* page, 
    int paperWidth, int paperHeight, int paperStride, 
    int rotation, int dpi)
{
    if (page == NULL) return NULL;

    int rotationDegree = 0;
    if (rotation == 0) {
        bool isCanvasLandscape = paperWidth > paperHeight;
        bool isPageLandscape = pdf_page_get_media_width(page) > pdf_page_get_media_height(page);
        rotationDegree = (isCanvasLandscape != isPageLandscape) ? 90 : 0;
    } else {
        rotationDegree = 90 * (rotation - 1);
    }

    int renderWidth = paperWidth;
    int renderHeight = paperHeight;
    int renderStride = paperStride;

    pdf_render_t* r = (pdf_render_t*)calloc(1, sizeof(pdf_render_t));
    r->page = page;
    r->pdf = page->pdf;
    r->current_obj = page->obj;

    r->pixels = (unsigned char*)calloc(renderStride * renderHeight, 1);
    memset(r->pixels, 0xFF, renderStride * renderHeight);

    r->width = renderWidth;
    r->height = renderHeight;
    r->stride = renderStride;

    pdf_deque_t* deque = pdf_deque_init();
    r->deque = deque;

    plutovg_surface_t* surface = plutovg_surface_create_for_data(r->pixels, renderWidth, renderHeight, renderStride);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);

    // clear background
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_restore(canvas);

    // get pdf page size (uint: pt)
    double pageWidth = pdf_page_get_media_width(page);
    double pageHeight = pdf_page_get_media_height(page);

    // convert canvas' px size to pt size
    double canvasWidthPt = renderWidth * 72.0 / dpi;
    double canvasHeightPt = renderHeight * 72.0 / dpi;

    // calc scale factor
    double scaleX = canvasWidthPt / (rotationDegree % 180 == 90 ? pageHeight : pageWidth);
    double scaleY = canvasHeightPt / (rotationDegree % 180 == 90 ? pageWidth : pageHeight);
    double scale = fmin(scaleX, scaleY); // fit to page

    // after scale, the actual content size
    double scaledWidth = pageWidth * scale;
    double scaledHeight = pageHeight * scale;

    // calc offset (in pt)
    double offsetX = (canvasWidthPt - scaledWidth) / 2.0;
    double offsetY = (canvasHeightPt - scaledHeight) / 2.0;

    // px -> pt -> rotate -> offset -> scale
    plutovg_canvas_translate(canvas, 0, renderHeight); // flip y in px
    plutovg_canvas_scale(canvas, dpi / 72.0, -(dpi / 72.0)); // px to pt

    // move to center then rotate
    plutovg_canvas_translate(canvas, canvasWidthPt / 2.0, canvasHeightPt / 2.0);
    plutovg_canvas_rotate(canvas, rotationDegree * M_PI / 180.0);
    plutovg_canvas_translate(canvas, -canvasWidthPt / 2.0, -canvasHeightPt / 2.0);

    // move and scale
    plutovg_canvas_translate(canvas, offsetX, offsetY);
    plutovg_canvas_scale(canvas, scale, scale);

    r->canvas = canvas;
    r->surface = surface;
    _init_state(r);
    r->fontcache = NULL;

    return r;
}
void pdf_render_free(pdf_render_t* context)
{
    if (context == NULL) return;
    pdf_deque_free(context->deque);
    for (int i = 0; i < cvector_size(context->fontcache); i++)
    {
        pdf_font_cache_t* fontcache = context->fontcache[i];
        if (fontcache->font != NULL)
        {
            pdf_font_free(fontcache->font);
        }
        if (fontcache->fontface != NULL)
        {
            plutovg_font_face_destroy(fontcache->fontface);
        }
        free(fontcache);
    }
    cvector_free(context->fontcache);
    free(context->state);
    plutovg_canvas_destroy(context->canvas);
    plutovg_surface_destroy(context->surface);
    free(context->pixels);
    free(context);
}
void pdf_render_save_to_png(pdf_render_t* context, char* filename)
{
    if (context == NULL || filename == NULL)
    {
        return;
    }
    
    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(context->pixels, context->width, context->height, context->stride);
    plutovg_surface_write_to_png(surface, filename);
    plutovg_surface_destroy(surface);
}
int pdf_render_copy_to_buffer(pdf_render_t* context, void* data, int len)
{
    if (context == NULL) return -1;
    int need = context->stride * context->height;
    if (data == NULL || len < need) return need;
    memcpy(data, context->pixels, need);
    return 0;
}
void _pdf_process_stream(pdf_render_t* context, pdf_stream_t* stream)
{
    pdf_stream_open(stream);
    pdf_parser_token_t* tk;
    //int count = 0;
    while ((tk = pdf_stream_get_next_token(stream)) != NULL)
    {
        if (tk->type == TOKEN_STREAM_END)
            break;
        _do_render_operation(stream, context, tk);
        pdf_parser_token_free(stream->parser, tk);
    }

    pdf_stream_close(stream);
}
void pdf_render_do(pdf_render_t* context)
{
    int numStreams = pdf_page_get_streams(context->page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(context->page, j);
        if (stream == NULL)
        {
            if (context->page->contents[j]->value->type == ARRAY)
            {
                pdf_array_t* arr = context->page->contents[j]->value->val.array;
                for (int i = 0; i < arr->num_elements; i++)
                {
                    if (arr->values[i]->type == INDIRECT)
                    {
                        pdf_obj_t* obj = pdf_file_get_obj(context->pdf, arr->values[i]->val.indirect);
                        if (obj != NULL && obj->stream != NULL)
                        {
                            _pdf_process_stream(context, obj->stream);
                        }
                    }
                }
            }
            else if (context->page->contents[j]->value->type == INDIRECT)
            {
                pdf_obj_t* obj = pdf_file_get_obj(context->pdf, context->page->contents[j]->value->val.indirect);
                if (obj != NULL && obj->stream != NULL)
                {
                    _pdf_process_stream(context, obj->stream);
                }
            }
            continue;
        }
            
        _pdf_process_stream(context, stream);
    }
    // Annots
    if (context->page->annots != NULL)
    {
        for (int i = 0; i < context->page->annots->num_elements; i++)
        {
            pdf_obj_t* anno_obj = pdf_file_get_obj(context->page->pdf, context->page->annots->values[i]->val.indirect);
            if (anno_obj != NULL)
            { 
                pdf_dict_get_name(anno_obj->value->val.dict, "/Type");
                pdf_dict_get_name(anno_obj->value->val.dict, "/SubType");
                pdf_array_t* rect_aar = pdf_dict_get_array(anno_obj->value->val.dict, "/Rect");
                if (rect_aar != NULL)
                {
                    plutovg_canvas_translate(context->canvas, rect_aar->values[0]->val.number, rect_aar->values[1]->val.number);
                }
                pdf_dict_get_string(anno_obj->value->val.dict, "/Contents");
                pdf_dict_get_dict(anno_obj->value->val.dict, "/P");
                pdf_dict_get_string(anno_obj->value->val.dict, "/NM");
                pdf_dict_get_string(anno_obj->value->val.dict, "/M");
                int F = pdf_dict_get_number(anno_obj->value->val.dict, "/F");
                if (F != -1)
                {
                    if (F & 0b0000000001)
                        ; // invisible
                    if (F & 0b0000000010)
                        ; // hidden
                    if (F & 0b0000000100)
                        ; // print
                    if (F & 0b0000001000)
                        ; // nozoom
                    if (F & 0b0000010000)
                        ; // norotate
                    if (F & 0b0000100000)
                        ; // noview
                    if (F & 0b0001000000)
                        ; // readonly
                    if (F & 0b0010000000)
                        ; // locked
                    if (F & 0b0100000000)
                        ; // togglenoview
                    if (F & 0b1000000000)
                        ; // lockedcontents
                }
                pdf_dict_t* AP = pdf_dict_get_dict(anno_obj->value->val.dict, "/AP");
                if (AP != NULL)
                {
                    pdf_dict_t* nomal_dict = pdf_dict_get_dict(AP, "/N"); // required
                    if (nomal_dict == NULL)
                    {
                        int ref = pdf_dict_get_ref(AP, "/N");
                        pdf_obj_t* obj = pdf_file_get_obj(context->page->pdf, ref);
                        if (obj->stream != NULL)
                        {
                            context->current_obj = obj;
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
    }
}

void _do_render_operation(pdf_stream_t* stream, pdf_render_t* context, pdf_parser_token_t* tk)
{
    if (tk != NULL)
    {
        if (tk->token != NULL)
        {
            printf("%s\n", tk->token);
        }
        else
        {
            printf("%s\n", _token_to_string(tk->type));
        }
    }

    if (tk->type < TOKEN_OPERATOR && tk->token != NULL)
    {
        // did not match any operation
        // push data to deque
        pdf_deque_push(context->deque, tk->token, tk->token_len);
    }
    else if (tk->type > TOKEN_OPERATOR && tk->type <= TOKEN_OPERATOR_y)
    {
        switch (tk->type)
        {
            case TOKEN_OPERATOR_B:
            case TOKEN_OPERATOR_F:
            case TOKEN_OPERATOR_W:
            case TOKEN_OPERATOR_b:
            case TOKEN_OPERATOR_f:
                plutovg_canvas_set_fill_rule(context->canvas,
                    PLUTOVG_FILL_RULE_NON_ZERO);
                break;
            case TOKEN_OPERATOR_B_star:
            case TOKEN_OPERATOR_W_star:
            case TOKEN_OPERATOR_b_star:
            case TOKEN_OPERATOR_f_star:
                plutovg_canvas_set_fill_rule(context->canvas,
                    PLUTOVG_FILL_RULE_EVEN_ODD);
                break;
            case TOKEN_OPERATOR_BI:
            {
                plutovg_canvas_save(context->canvas);
                pdf_parser_token_t* t = NULL;
                int width = 0, height = 0;
                int channels = 3;
                int bitsPerColor = 8;
                char filter[64] = {0};
                while ((t = pdf_stream_get_next_token(stream)) != NULL)
                {
                    if (t->type == TOKEN_OPERATOR_ID)
                    {
                        pdf_parser_token_free(stream->parser, t);
                        break;
                    }
                    else if (t->type == TOKEN_NAME)
                    {
                        pdf_parser_token_t* t1 = pdf_stream_get_next_token(stream);
                        if (t1 != NULL)
                        {
                            if (!strcmp(t->token, "/W"))
                            {
                                width = atoi(t1->token);
                            }
                            else if (!strcmp(t->token, "/H"))
                            {
                                height = atoi(t1->token);
                            }
                            else if (!strcmp(t->token, "/BPC"))
                            {
                                bitsPerColor = atoi(t1->token);
                            }
                            else if (!strcmp(t->token, "/CS"))
                            {
                                if (!strcmp(t1->token, "/RGB"))
                                {
                                    channels = 3;
                                }
                                else if (!strcmp(t1->token, "/Gray"))
                                {
                                    channels = 1;
                                }
                            }
                            else if (!strcmp(t->token, "/F"))
                            {
                                if (t1->type == TOKEN_NAME)
                                {
                                    strcpy(filter, t1->token);
                                }
                                else if (t1->type == TOKEN_ARRAY_BEG)
                                {
                                    pdf_parser_token_t* t2 = pdf_stream_get_next_token(stream);
                                    if (t2 != NULL && t2->type == TOKEN_NAME)
                                    {
                                        strcpy(filter, t2->token);
                                    }
                                    pdf_parser_token_free(stream->parser, t2);
                                    pdf_parser_token_t* t3 = pdf_stream_get_next_token(stream);
                                    if (t3->type == TOKEN_ARRAY_END)
                                    {

                                    }
                                    pdf_parser_token_free(stream->parser, t3);

                                }
                            }
                            pdf_parser_token_free(stream->parser, t1);
                        }
                    }
 
                    pdf_parser_token_free(stream->parser, t);
                }
                int size = width * channels * height;
                char* buffer = (char*)malloc(size);
                size_t ret = 0;
                if (!strcmp(filter, "/DCT"))
                {
                    char c;
                    while (pdf_parser_read_data(stream->parser, &c, 1) == 1)
                    {
                        memcpy(buffer + ret, &c, 1);
                        ret += 1;
                        if (ret >= 2 && !memcmp(buffer + ret - 2, "\xFF\xD9", 2))
                        {
                            pdf_parser_token_t* t1 = pdf_stream_get_next_token(stream);
                            if (t1 != NULL && t1->type == TOKEN_OPERATOR_EI)
                            {
                                pdf_parser_token_free(stream->parser, t1);
                                break;
                            }
                        }
                    }
                    size = ret;
                }
                else
                {
                    ret = pdf_parser_read_data(stream->parser, buffer, size);
                }
                
                plutovg_surface_t* s = NULL;
                if (memcmp(buffer, "\xFF\xD8", 2) == 0) // jpeg
                {
                    s = plutovg_surface_load_from_image_data(buffer,
                        size);
                    // unsigned char* pixels = stbi_load_from_memory(xobj->image->data,
                    // xobj->image->data_len, &width, &height, &channels, channels);
                    // //convert_to_gray(pixels, width, height, width * channels, channels);
                    // //average_gray(pixels, width, height, width * channels, channels);
                    // //dither_by_threshold(pixels, width, height, width * channels,
                    // channels, 220); free(xobj->image->data); xobj->image->data = pixels;
                    // xobj->image->data_len = width * channels * height;
                    // dither_by_threshold(pixels, width, height, width * channels, 220);
                    // stbi_image_free(pixels);
                }
                else if (memcmp(buffer, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",
                    8) == 0) // png
                {
                    s = plutovg_surface_load_from_image_data(buffer,
                        size);
                }
                else if (memcmp(buffer, "P6", 2) == 0) // ppm
                {
                    s = plutovg_surface_load_from_image_data(buffer,
                        size);
                }
                else
                {
                    // tje_encode_to_file("tje.jpg", width, height, channels,
                    // xobj->image->data);
                    //  add header
                    char header[128] = { 0 };
                    if (channels == 3)
                    {
                        sprintf(header, "P6 %d %d 255\n", width, height);
                    }
                    else if (channels == 1)
                    {
                        sprintf(header, "P5 %d %d 255\n", width, height);
                    }

                    int header_len = strlen(header);
                    int actual_line_bytes = size / height;
                    int actual_data_len = size;
                    int real_line_bytes = channels * width;
                    int real_data_len = real_line_bytes * height;
                    if (channels == 3 && real_data_len != actual_data_len)
                    {
                        unsigned char* tmp =
                            (unsigned char*)malloc(real_data_len + header_len);
                        int off = 0;
                        memcpy(tmp, header, header_len);
                        off += header_len;
                        for (int i = 0; i < height; i++)
                        {
                            memcpy(tmp + off, buffer + i * actual_line_bytes,
                                real_line_bytes);
                            off += real_line_bytes;
                        }
                        free(buffer);
                        buffer = tmp;
                        size = off;
                    }
                    else
                    {
                        unsigned char* tmp =
                            (unsigned char*)malloc(buffer + header_len);
                        int off = 0;
                        memcpy(tmp, header, header_len);
                        off += header_len;
                        memcpy(tmp + off, buffer, size);
                        off += size;
                        free(buffer);
                        buffer = tmp;
                        size = off;
                    }

                    s = plutovg_surface_load_from_image_data(buffer,
                        size);
                }
                if (s == NULL)
                {
                    printf("process BI-ID failed\n");
                    return;
                }
                float scale_x = 1.f / width;
                float scale_y = 1.f / height;
                // Transformation matrix to scale and flip the image vertically
                plutovg_matrix_t m = { scale_x,  0,
                                        0, -scale_y,
                                        0, height * scale_y };

                plutovg_canvas_set_texture(context->canvas, s, PLUTOVG_TEXTURE_TYPE_PLAIN,
                    1.0f, &m);
                plutovg_canvas_paint(context->canvas);
                plutovg_surface_destroy(s);
                free(buffer);

                plutovg_canvas_restore(context->canvas);
                break;
            }
            default:
                break;
        }
        if (handlers[tk->type - TOKEN_OPERATOR])
        {
            handlers[tk->type - TOKEN_OPERATOR](context);
        }
        // if (tk->type == TOKEN_OPERATOR_TJ || tk->type == TOKEN_OPERATOR_Tj)
        // {
        //     plutovg_surface_write_to_png(context->surface, "test.png");
        //     getchar();
        // }
    }
    else
    {
        printf("unknow token %d\n", tk->type);
    }
}

void handle_BDC(pdf_render_t* context)
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
// void handle_BI(pdf_render_t* context)
// {
//     // begin an inline image object
// }

void handle_BMC(pdf_render_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}


void handle_cm(pdf_render_t* context)
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



void handle_d(pdf_render_t* context)
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

void handle_d0(pdf_render_t* context)
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

void handle_d1(pdf_render_t* context)
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

void handle_DP(pdf_render_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
}
// void handle_EI(pdf_render_t* context)
// {
//     // end an inline image object
// }

void handle_EMC(pdf_render_t* context) {}

void handle_gs(pdf_render_t* context)
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

void handle_i(pdf_render_t* context)
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

// void handle_ID(pdf_render_t* context)
// {
//     // begin the image data for an inline image object
// }

void handle_j(pdf_render_t* context)
{
    // set join style
    // lineJoin

    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    int j = atoi(buf);

    plutovg_canvas_set_line_join(context->canvas, j);
    context->state->lineJoin = j;
}

void handle_J(pdf_render_t* context)
{
    // set cap style
    // lineCap
    pdf_node_t node;
    char buf[1024] = { 0 };
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    int c = atoi(buf);

    plutovg_canvas_set_line_cap(context->canvas, c);
    context->state->lineCap = c;
}


void handle_M(pdf_render_t* context)
{
    // set miter limit
    // miterLimit
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float m = strtof(buf, NULL);
    plutovg_canvas_set_miter_limit(context->canvas, m);
    context->state->miterLimit = m;
}

void handle_MP(pdf_render_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}

void handle_q(pdf_render_t* context)
{
    // store state
    plutovg_canvas_save(context->canvas);
    if (context->state == NULL) _init_state(context);
    pdf_graphics_state_t* new_state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memcpy(new_state, context->state, sizeof(pdf_graphics_state_t));
    new_state->next = context->state;
    context->state = new_state;
}

void handle_Q(pdf_render_t* context)
{
    // restore state
    plutovg_canvas_restore(context->canvas);
    pdf_graphics_state_t* old_state = context->state;
    context->state = old_state->next;
    free(old_state);
    if (context->state == NULL) _init_state(context);
    plutovg_canvas_set_line_width(context->canvas, context->state->lineWidth);
    plutovg_canvas_set_line_cap(context->canvas, (plutovg_line_cap_t)context->state->lineCap);
    plutovg_canvas_set_line_join(context->canvas, (plutovg_line_join_t)context->state->lineJoin);
    plutovg_canvas_set_miter_limit(context->canvas, context->state->miterLimit);
}


void handle_ri(pdf_render_t* context)
{
    // set color rendering intent
    // intent
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}

void handle_sh(pdf_render_t* context)
{
    // name
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
}