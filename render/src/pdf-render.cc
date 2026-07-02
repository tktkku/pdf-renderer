#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
#include <cstddef>
#include <math.h>
#include <typeinfo>
#include <algorithm>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
void _init_state(pdf_render* context)
{
    // ponytail: use pre-allocated state stack (PDF max q/Q nesting is 28)
    context->state_stack_depth = 0;
    context->state = &context->state_stack[0];
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
            fontcache->font = NULL;
        }
        if (fontcache->fontface != NULL)
        {
            pdf_font_face_destroy(fontcache->fontface);
            fontcache->fontface = NULL;
        }
        delete fontcache;
    }
    context->fontcache.clear();
    // ponytail: state is now on pre-allocated state_stack, no free needed
    delete context;
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
                    pdf_render_command* cmd = new pdf_render_command(TOKEN_OPERATOR_Td);
                    cmd->matrix.a = tx;
                    cmd->matrix.b = ty;
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
                        pdf_indirect_t ref = AP->get_indirect("/N");
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
#define DEBUG_TOKEN 0
#if DEBUG_TOKEN
    if (tk != NULL)
    {
        if (tk->size() > 0)
        {
            printf("%s ", tk->data());
        }
        else
        {
            printf("%s ", _token_to_string(tk->type()));
        }
    }
#endif
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
            pdf_render_command* cmd = new pdf_render_command(tk->type());
            handlers[tk->type() - TOKEN_OPERATOR](context, cmd, true);
#if DEBUG_TOKEN
            printf("\n");
#endif
            return cmd;
        }
    }
    else
    {
        printf("unknow token %d\n", tk->type());
    }
    return NULL;
}
void pdf_render_run(pdf_render* render, pdf_renderer_t* renderer)
{
    render->renderer = renderer;
    for (auto& c : render->operations)
    {
        if (handlers[c->type - TOKEN_OPERATOR])
        {
            // printf("run %s\n", _token_to_string(c->type));
            handlers[c->type - TOKEN_OPERATOR](render, c.get(), false);
        }
    }
}
