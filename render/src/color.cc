#include "pdf-render.h"
#include "pdf-render-private.h"
void handle_cs(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_cs;
        memcpy(cmd->colorSpace, data->data(), data->size());
    }
    else
    {
        if (!strcmp(cmd->colorSpace, "/DeviceGray") || !strcmp(cmd->colorSpace, "/DeviceRGB") || !strcmp(cmd->colorSpace, "/DeviceCMYK"))
        {
            strcpy(context->state->fill.currentColorSpace, cmd->colorSpace);
        }
        else
        {
            pdf_obj_get_colorspace(context->current_obj, cmd->colorSpace, context->state->fill.currentColorSpace);
        }
    }
}

void handle_CS(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set color space
    // /DeviceGray
    // initialize the corresponding current color to 0.0
    /// DeviceRGB
    // initialize the corresponding current color of red green blue to 0.0
    // /DeviceCMYK
    // initialize the corresponding current color of cyan magenta yellow to 0.0
    // and the black to 1.0
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_CS;
        memcpy(cmd->colorSpace, data->data(), data->size());
    }
    else
    {
        if (!strcmp(cmd->colorSpace, "/DeviceGray") || !strcmp(cmd->colorSpace, "/DeviceRGB") || !strcmp(cmd->colorSpace, "/DeviceCMYK"))
        {
            strcpy(context->state->stroke.currentColorSpace, cmd->colorSpace);
        }
        else
        {
            pdf_obj_get_colorspace(context->current_obj, cmd->colorSpace, context->state->stroke.currentColorSpace);
        }
    }
}

void handle_g(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // for nonstroking
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float g = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_g;
        cmd->color.g = g;
    }
    else
    {
        context->state->fill.color[0] = cmd->color.g;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.g;
        strcpy(context->state->fill.currentColorSpace, "/DeviceGray");
    }
}

void handle_G(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set both in one operation
    // gray
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float g = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_G;
        cmd->color.g = g;
    }
    else
    {
        context->state->stroke.color[0] = cmd->color.g;
        context->state->stroke.color[1] = cmd->color.g;
        context->state->stroke.color[2] = cmd->color.g;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceGray");
    }
}

void handle_k(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // for nonstroking

    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float k = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float y = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float m = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float c = strtof(data4->data(), NULL);
        // handle_K(context);
        float r = (1.0 - c) * (1.0 - k);
        float g = (1.0 - m) * (1.0 - k);
        float b = (1.0 - y) * (1.0 - k);
        cmd->type = TOKEN_OPERATOR_k;
        cmd->color.r = r;
        cmd->color.g = g;
        cmd->color.b = b;
    }
    else
    {
        context->state->fill.color[0] = cmd->color.r;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.b;
        strcpy(context->state->fill.currentColorSpace, "/DeviceCMYK");
    }
}

void handle_K(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // combine CS and SC for DeviceCMYK
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float k = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float y = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float m = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float c = strtof(data4->data(), NULL);
        float r = (1.0 - c) * (1.0 - k);
        float g = (1.0 - m) * (1.0 - k);
        float b = (1.0 - y) * (1.0 - k);
        cmd->type = TOKEN_OPERATOR_K;
        cmd->color.r = r;
        cmd->color.g = g;
        cmd->color.b = b;
    }
    else
    {
        context->state->stroke.color[0] = cmd->color.r;
        context->state->stroke.color[1] = cmd->color.g;
        context->state->stroke.color[2] = cmd->color.b;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceCMYK");
    }
}

void handle_rg(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // for nonstroking
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float b = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float g = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float r = strtof(data3->data(), NULL);
        cmd->type = TOKEN_OPERATOR_rg;
        cmd->color.r = r;
        cmd->color.g = g;
        cmd->color.b = b;
    }
    else
    {
        context->state->fill.color[0] = cmd->color.r;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.b;
        strcpy(context->state->fill.currentColorSpace, "/DeviceRGB");
    }
}


void handle_RG(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // combine CS and SC for DeviceRGB
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float b = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float g = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float r = strtof(data3->data(), NULL);
        cmd->type = TOKEN_OPERATOR_RG;
        cmd->color.r = r;
        cmd->color.g = g;
        cmd->color.b = b;
    }
    else
    {
        context->state->stroke.color[0] =  cmd->color.r;
        context->state->stroke.color[1] =  cmd->color.g;
        context->state->stroke.color[2] =  cmd->color.b;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceRGB");
    }
}

void handle_sc(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);

    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray"))
    {
        // gray
        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float g = strtof(data->data(), NULL);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.g = g;
        }
        else
        {
            context->state->fill.color[0] = cmd->color.g;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.g;
        }
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue
        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float b = strtof(data->data(), NULL);
            auto data2 = context->deque->pop_front();
            float g = strtof(data2->data(), NULL);
            auto data3 = context->deque->pop_front();
            float r = strtof(data3->data(), NULL);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.r = r;
            cmd->color.g = g;
            cmd->color.b = b;
        }
        else
        {
            context->state->fill.color[0] = cmd->color.r;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.b;
        }
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float k = strtof(data->data(), NULL);
            auto data2 = context->deque->pop_front();
            float y = strtof(data2->data(), NULL);
            auto data3 = context->deque->pop_front();
            float m = strtof(data3->data(), NULL);
            auto data4 = context->deque->pop_front();
            float c = strtof(data4->data(), NULL);

            float r = (1.0 - c) * (1.0 - k);
            float g = (1.0 - m) * (1.0 - k);
            float b = (1.0 - y) * (1.0 - k);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.r = r;
            cmd->color.g = g;
            cmd->color.b = b;
        }
        else
        {
            context->state->fill.color[0] = cmd->color.r;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.b;
        }
    }
}


void handle_SC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set gray level, 0.0 to balck 1.0 to white
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray"))
    {
        // gray
        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float g = strtof(data->data(), NULL);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.g = g;
        }
        else
        {
            context->state->stroke.color[0] = cmd->color.g;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.g;
        }
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float b = strtof(data->data(), NULL);
            auto data2 = context->deque->pop_front();
            float g = strtof(data2->data(), NULL);
            auto data3 = context->deque->pop_front();
            float r = strtof(data3->data(), NULL);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.r = r;
            cmd->color.g = g;
            cmd->color.b = b;
        }
        else
        {
            context->state->stroke.color[0] = cmd->color.r;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.b;
        }
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
        if (dry_run)
        {
            auto data = context->deque->pop_front();
            float k = strtof(data->data(), NULL);
            auto data2 = context->deque->pop_front();
            float y = strtof(data2->data(), NULL);
            auto data3 = context->deque->pop_front();
            float m = strtof(data3->data(), NULL);
            auto data4 = context->deque->pop_front();
            float c = strtof(data4->data(), NULL);

            float r = (1.0 - c) * (1.0 - k);
            float g = (1.0 - m) * (1.0 - k);
            float b = (1.0 - y) * (1.0 - k);
            cmd->type = TOKEN_OPERATOR_sc;
            cmd->color.r = r;
            cmd->color.g = g;
            cmd->color.b = b;
        }
        else
        {
            context->state->stroke.color[0] = cmd->color.r;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.b;
        }
    }
}

void handle_scn(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // char buf[1024] = { 0 };
    // pdf_node_t node;
    // node.data = buf;
    // pdf_deque_pop_front(context->deque, &node);
    // pdf_deque_pop_front(context->deque, &node);
    // pdf_deque_pop_front(context->deque, &node);
    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_sc(context, cmd, dry_run);
    }
    auto data = context->deque->pop_front();
    auto data2 = context->deque->pop_front();
    auto data3 = context->deque->pop_front();
}

void handle_SCN(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_SC(context, cmd, dry_run);
    }
  
    auto data = context->deque->pop_front();
    auto data2 = context->deque->pop_front();
    auto data3 = context->deque->pop_front();
}