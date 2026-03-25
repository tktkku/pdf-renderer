#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
#include <cstddef>
#include <plutovg-private.h>
#include <math.h>
#include <typeinfo>
#include <wingdi.h>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void _init_state(pdf_render* context)
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
pdf_render* pdf_render_init(pdf_page_t* page)
{
    if (page == NULL) return NULL;
    pdf_render* r = new pdf_render();
    r->page = page;
    r->pdf = page->pdf;
    r->current_obj = page->obj;
    pdf_deque* deque = new pdf_deque();
    r->deque = deque;
    _init_state(r);

    return r;
}

void pdf_render_free(pdf_render* context)
{
    if (context == NULL) return;
    delete context->deque;
    for (size_t i = 0; i < context->fontcache.size(); i++)
    {
        pdf_font_cache_t* fontcache = context->fontcache[i];
        if (fontcache->font != NULL)
        {
            pdf_font_free(fontcache->font);
        }
        if (fontcache->fontface != NULL)
        {
            pdf_font_face_destroy(fontcache->fontface);
        }
        delete fontcache;
    }
    context->fontcache.clear();
    free(context->state);
    plutovg_canvas_destroy(context->canvas);
    plutovg_surface_destroy(context->surface);
    free(context->pixels);
    delete context;
}
void pdf_render_save_to_png(pdf_render* context, char* filename)
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
int pdf_render_copy_to_buffer(pdf_render* context, void* data, int len)
{
    if (context == NULL) return -1;
    int need = context->stride * context->height;
    if (data == NULL || len < need) return need;
    memcpy(data, context->pixels, need);
    return 0;
}
void _pdf_process_stream(pdf_render* context, pdf_stream_t* stream)
{
    pdf_stream_open(stream);
    
    while (true)
    {
        pdf_token* tk = pdf_parser_next_token(stream->parser);
        if (tk == NULL || tk->type() == TOKEN_STREAM_END)
        {
            break;
        }
        pdf_render_command* cmd = _do_render_operation(stream, context, tk);
        if (cmd)
        {
            std::unique_ptr<pdf_render_command> u(cmd);
            context->operations.push_back(std::move(u));
        }
        
        delete tk;
    }

    pdf_stream_close(stream);
}
void pdf_render_build(pdf_render* context)
{
    int numStreams = pdf_page_get_streams(context->page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(context->page, j);
        if (stream == NULL)
        {
            if (context->page->contents[j]->value->type == PDF_VALUE_ARRAY)
            {
                pdf_array* arr = context->page->contents[j]->value->val.array;
                for (size_t i = 0; i < arr->size(); i++)
                {
                    if (arr->get(i)->type == PDF_VALUE_INDIRECT)
                    {
                        pdf_obj_t* obj = pdf_file_get_obj(context->pdf, arr->get(i)->val.indirect);
                        if (obj != NULL && obj->stream != NULL)
                        {
                            _pdf_process_stream(context, obj->stream);
                        }
                    }
                }
            }
            else if (context->page->contents[j]->value->type == PDF_VALUE_INDIRECT)
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
        for (size_t i = 0; i < context->page->annots->size(); i++)
        {
            pdf_obj_t* anno_obj = pdf_file_get_obj(context->page->pdf, context->page->annots->get(i)->val.indirect);
            if (anno_obj != NULL)
            {
                anno_obj->value->val.dict->get_name("/Type");
                anno_obj->value->val.dict->get_name("/SubType");
                pdf_array* rect_aar = anno_obj->value->val.dict->get_array("/Rect");
                if (rect_aar != NULL)
                {
                    float tx = rect_aar->get(0)->val.number;
                    float ty = rect_aar->get(1)->val.number;
                    pdf_render_command* cmd = new pdf_render_command;
                    cmd->type = TOKEN_OPERATOR_Td;
                    cmd->Td.x = tx;
                    cmd->Td.y = ty;
                    std::unique_ptr<pdf_render_command> u(cmd);
                    context->operations.push_back(std::move(u));
                }
                anno_obj->value->val.dict->get_string("/Contents");
                anno_obj->value->val.dict->get_dict("/P");
                anno_obj->value->val.dict->get_string("/NM");
                anno_obj->value->val.dict->get_string("/M");
                int F = anno_obj->value->val.dict->get_number("/F");
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
                pdf_dict* AP = anno_obj->value->val.dict->get_dict("/AP");
                if (AP != NULL)
                {
                    pdf_dict* nomal_dict = AP->get_dict("/N"); // required
                    if (nomal_dict == NULL)
                    {
                        int ref = AP->get_indirect("/N");
                        pdf_obj_t* obj = pdf_file_get_obj(context->page->pdf, ref);
                        if (obj->stream != NULL)
                        {
                            context->current_obj = obj;
                            pdf_stream_open(obj->stream);
                            pdf_token* tk = NULL;
                            while ((tk = pdf_parser_next_token(obj->stream->parser)) != NULL)
                            {
                                if (tk->type() == TOKEN_STREAM_END)
                                    break;
                                pdf_render_command* cmd = _do_render_operation(obj->stream, context, tk);
                                if (cmd)
                                {
                                    std::unique_ptr<pdf_render_command> u(cmd);
                                    context->operations.push_back(std::move(u));
                                }
                                
                                delete tk;
                            }
                            pdf_stream_close(obj->stream);
                        }
                    }
                }
                anno_obj->value->val.dict->get_name("/AS");
                anno_obj->value->val.dict->get_array("/Border");
                anno_obj->value->val.dict->get_array("/C");
                anno_obj->value->val.dict->get_number("/StructParent");
                anno_obj->value->val.dict->get_dict("/OC");
            }
        }
    }
}

pdf_render_command* _do_render_operation(pdf_stream_t* stream, pdf_render* context, pdf_token* tk)
{
    // if (tk != NULL)
    // {
    //     if (tk->size() > 0)
    //     {
    //         printf("%s\n", tk->data());
    //     }
    //     else
    //     {
    //         printf("%s\n", _token_to_string(tk->type()));
    //     }
    // }

    if (tk->type() < TOKEN_OPERATOR && !tk->empty())
    {
        // did not match any operation
        // push data to deque
        context->deque->push_front(tk->data(), tk->size());
    }
    else if (tk->type() > TOKEN_OPERATOR && tk->type() <= TOKEN_OPERATOR_y)
    {
        switch (tk->type())
        {
            case TOKEN_OPERATOR_BI:
            {
                pdf_token* t = NULL;
                int width = 0, height = 0;
                int channels = 3;
                int bitsPerColor = 8;
                char filter[64] = {0};
                while ((t = pdf_parser_next_token(stream->parser)) != NULL)
                {
                    if (t->type() == TOKEN_OPERATOR_ID)
                    {
                        delete t;
                        break;
                    }
                    else if (t->type() == TOKEN_NAME)
                    {
                        pdf_token* t1 = pdf_parser_next_token(stream->parser);
                        if (t1 != NULL)
                        {
                            if (!strcmp(t->data(), "/W"))
                            {
                                width = atoi(t1->data());
                            }
                            else if (!strcmp(t->data(), "/H"))
                            {
                                height = atoi(t1->data());
                            }
                            else if (!strcmp(t->data(), "/BPC"))
                            {
                                bitsPerColor = atoi(t1->data());
                            }
                            else if (!strcmp(t->data(), "/CS"))
                            {
                                if (!strcmp(t1->data(), "/RGB"))
                                {
                                    channels = 3;
                                }
                                else if (!strcmp(t1->data(), "/Gray"))
                                {
                                    channels = 1;
                                }
                            }
                            else if (!strcmp(t->data(), "/F"))
                            {
                                if (t1->type() == TOKEN_NAME)
                                {
                                    strcpy(filter, t1->data());
                                }
                                else if (t1->type() == TOKEN_ARRAY_BEG)
                                {
                                    pdf_token* t2 = pdf_parser_next_token(stream->parser);
                                    if (t2 != NULL && t2->type() == TOKEN_NAME)
                                    {
                                        strcpy(filter, t2->data());
                                    }
                                    delete t2;
                                    pdf_token* t3 = pdf_parser_next_token(stream->parser);
                                    if (t3->type() == TOKEN_ARRAY_END)
                                    {

                                    }
                                    delete t3;

                                }
                            }
                            delete t1;
                        }
                    }
 
                    delete t;
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
                            pdf_token* t1 = pdf_parser_next_token(stream->parser);
                            if (t1 != NULL && t1->type() == TOKEN_OPERATOR_EI)
                            {
                                delete t1;
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
                context->deque->push_front(&size, sizeof(size));
                context->deque->push_front(&channels, sizeof(channels));
                context->deque->push_front(&height, sizeof(height));
                context->deque->push_front(&width, sizeof(width));
                context->deque->push_front(&buffer, sizeof(char*));
                break;
            }
            default:
                break;
        }
        if (handlers[tk->type() - TOKEN_OPERATOR])
        {
            pdf_render_command* cmd = new pdf_render_command;
            handlers[tk->type() - TOKEN_OPERATOR](context, cmd, true);
            return cmd;
        }
    }
    else
    {
        printf("unknow token %d\n", tk->type());
    }
    return NULL;
}
void pdf_render_with_size(pdf_render* context, int width, int height, int stride, int dpi)
{
    unsigned char* pixels = (unsigned char*)malloc(stride * height);
    memset(pixels, 0xFF, stride * height);
    context->pixels = pixels;
    context->width = width;
    context->height = height;
    context->stride = stride;

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
    context->canvas = canvas;
    context->surface = surface;

    for (auto& c : context->operations)
    {
        handlers[c->type - TOKEN_OPERATOR](context, c.get(), false);
    }
}
void pdf_render_for_paper(pdf_render* context, 
    int paperWidth, int paperHeight, int paperStride, 
    int rotation, int dpi)
{
    if (context == NULL) return;

    int rotationDegree = 0;
    if (rotation == 0) {
        bool isCanvasLandscape = paperWidth > paperHeight;
        bool isPageLandscape = pdf_page_get_media_width(context->page) > pdf_page_get_media_height(context->page);
        rotationDegree = (isCanvasLandscape != isPageLandscape) ? 90 : 0;
    } else {
        rotationDegree = 90 * (rotation - 1);
    }

    int renderWidth = paperWidth;
    int renderHeight = paperHeight;
    int renderStride = paperStride;

    context->pixels = (unsigned char*)calloc(renderStride * renderHeight, 1);
    memset(context->pixels, 0xFF, renderStride * renderHeight);

    context->width = renderWidth;
    context->height = renderHeight;
    context->stride = renderStride;

    plutovg_surface_t* surface = plutovg_surface_create_for_data(context->pixels, renderWidth, renderHeight, renderStride);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);

    // clear background
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_restore(canvas);

    // get pdf page size (uint: pt)
    double pageWidth = pdf_page_get_media_width(context->page);
    double pageHeight = pdf_page_get_media_height(context->page);

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

    context->canvas = canvas;
    context->surface = surface;

    for (auto& c : context->operations)
    {
        handlers[c->type - TOKEN_OPERATOR](context, c.get(), false);
    }
}
void handle_BDC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag properties
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        if (!strcmp(data->data(), ">>"))
        {
            context->deque->pop_front(); // str
            context->deque->pop_front(); // name
            context->deque->pop_front(); // <<
        }
        else
        {
            context->deque->pop_front(); // name
        }
        context->deque->pop_front(); // tag
        cmd->type = TOKEN_OPERATOR_BDC;
    }
}
void handle_BI(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // begin an inline image object
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        char** pptr = ((char**)(void*)data->data());
        char* buffer = *pptr;

        auto data1 = context->deque->pop_front();
        int* ptr1 = ((int*)(void*)data1->data());
        int width = *ptr1;
        
        auto data2 = context->deque->pop_front();
        int* ptr2 = ((int*)(void*)data2->data());
        int height = *ptr2;

        auto data3 = context->deque->pop_front();
        int* ptr3 = ((int*)(void*)data3->data());
        int channels = *ptr3;

        auto data4 = context->deque->pop_front();
        int* ptr4 = ((int*)(void*)data4->data());
        int size = *ptr4;
  
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
                buffer = (char*)tmp;
                size = off;
            }
            else
            {
                unsigned char* tmp =
                    (unsigned char*)malloc(size + header_len);
                int off = 0;
                memcpy(tmp, header, header_len);
                off += header_len;
                memcpy(tmp + off, buffer, size);
                off += size;
                free(buffer);
                buffer = (char*)tmp;
                size = off;
            }

            s = plutovg_surface_load_from_image_data(buffer,
                size);
        }
        cmd->type = TOKEN_OPERATOR_Do;
        cmd->Do.type = XOBJ_IMAGE;
        cmd->Do.width = width;
        cmd->Do.height = height;
        cmd->Do.surface = s;
    }
}

void handle_BMC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag
    if (dry_run)
    {
        context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_BMC;
    }
}


void handle_cm(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // change matrix CTM
    // a b c d e f
    if (dry_run)
    {
        char buf[1024] = { 0 };
        auto data = context->deque->pop_front();
        float f = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float e = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float d = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float c = strtof(data4->data(), NULL);
        auto data5 = context->deque->pop_front();
        float b = strtof(data5->data(), NULL);
        auto data6 = context->deque->pop_front();
        float a = strtof(data6->data(), NULL);
        cmd->type = TOKEN_OPERATOR_cm;
        cmd->cm.x1 = a;
        cmd->cm.y1 = b;
        cmd->cm.x2 = c;
        cmd->cm.y2 = d;
        cmd->cm.x3 = e;
        cmd->cm.y3 = f;
    }
    else
    {
        plutovg_matrix_t ctm;
        plutovg_matrix_init(&ctm, 
            cmd->cm.x1, cmd->cm.y1, cmd->cm.x2, cmd->cm.y2, cmd->cm.x3, cmd->cm.y3);
        plutovg_canvas_transform(context->canvas, &ctm);
    }
}



void handle_d(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set line dash pattern
    // dashArray dashPhase
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float offset = strtof(data->data(), NULL);

        int index = 2;
        float dashs[2] = { 0 };
        while (true)
        {
            auto data2 = context->deque->pop_front();
            if (!strcmp(data2->data(), "]"))
            {
                // ignore
            }
            else if (!strcmp(data2->data(), "["))
            {
                break;
            }
            else
            {
                float dash = strtof(data2->data(), NULL);
                if (index > 0)
                    dashs[--index] = dash;
            }
        }
        cmd->type = TOKEN_OPERATOR_d;
        cmd->d.offset = offset;
        cmd->d.dashs[0] = dashs[0];
        cmd->d.dashs[1] = dashs[1];
    }
    else
    {
        plutovg_canvas_set_dash_offset(context->canvas, cmd->d.offset);
        plutovg_canvas_set_dash_array(context->canvas, cmd->d.dashs, 2);
    }
}

void handle_d0(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // wx wy
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float wy = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float wx = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_d0;
    }
}

void handle_d1(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // wx wy llx lly urx ury
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float ury = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float urx = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float lly = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float llx = strtof(data4->data(), NULL);
        auto data5 = context->deque->pop_front();
        float wy = strtof(data5->data(), NULL);
        auto data6 = context->deque->pop_front();
        float wx = strtof(data6->data(), NULL);
        cmd->type = TOKEN_OPERATOR_d1;
    }
}

void handle_DP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag properties
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        auto data2 = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_DP;
    }
}
// void handle_EI(pdf_render* context, pdf_render_command* cmd, bool dry_run)
// {
//     // end an inline image object
// }

void handle_EMC(pdf_render* context, pdf_render_command* cmd, bool dry_run) {
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_EMC;
    }
}

void handle_gs(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set specified parameters
    // dictName shall be the name of
    // a graphics state parameter dictionary
    // in the ExtGState subdictionary of the current resource dictionary
    // dictName
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_gs;
    }
}

void handle_i(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set flatness tolerance
    // flatness
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_i;
    }
}

// void handle_ID(pdf_render* context, pdf_render_command* cmd, bool dry_run)
// {
//     // begin the image data for an inline image object
// }

void handle_j(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set join style
    // lineJoin
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int j = atoi(data->data());
        cmd->type = TOKEN_OPERATOR_j;
        cmd->j.i = j;
    }
    else
    {
        plutovg_canvas_set_line_join(context->canvas, (plutovg_line_join_t)cmd->j.i);
        context->state->lineJoin = cmd->j.i;
    }
}

void handle_J(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set cap style
    // lineCap
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int c = atoi(data->data());
        cmd->type = TOKEN_OPERATOR_J;
        cmd->J.i = c;
    }
    else
    {
        plutovg_canvas_set_line_cap(context->canvas, (plutovg_line_cap_t)cmd->J.i);
        context->state->lineCap = cmd->J.i;
    }
}


void handle_M(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set miter limit
    // miterLimit
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float m = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_M;
        cmd->M.f = m;
    }
    else
    {
        plutovg_canvas_set_miter_limit(context->canvas, cmd->M.f);
        context->state->miterLimit = cmd->M.f;
    }
}
void handle_MP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_MP;
    }
}

void handle_q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // store state
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_q;
    }
    else
    {
        plutovg_canvas_save(context->canvas);
        if (context->state == NULL) _init_state(context);
        pdf_graphics_state_t* new_state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
        memcpy(new_state, context->state, sizeof(pdf_graphics_state_t));
        new_state->next = context->state;
        context->state = new_state;
    }
}

void handle_Q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // restore state
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_Q;
    }
    else
    {
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
}


void handle_ri(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set color rendering intent
    // intent
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_ri;
    }
}

void handle_sh(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // name
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_sh;
    }
}