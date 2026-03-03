#include "pdf-render.h"
#include "pdf-render-private.h"
void handle_cs(pdf_render* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);

    auto data = context->deque->pop_front();
    if (!strcmp(data->data(), "/DeviceGray") || !strcmp(data->data(), "/DeviceRGB") || !strcmp(data->data(), "/DeviceCMYK"))
    {
        strcpy(context->state->fill.currentColorSpace, data->data());
    }
    else
    {
        pdf_obj_get_colorspace(context->current_obj, data->data(), context->state->fill.currentColorSpace);
    }
}

void handle_CS(pdf_render* context)
{
    // set color space
    // /DeviceGray
    // initialize the corresponding current color to 0.0
    /// DeviceRGB
    // initialize the corresponding current color of red green blue to 0.0
    // /DeviceCMYK
    // initialize the corresponding current color of cyan magenta yellow to 0.0
    // and the black to 1.0
    auto data = context->deque->pop_front();
    if (!strcmp(data->data(), "/DeviceGray") || !strcmp(data->data(), "/DeviceRGB") || !strcmp(data->data(), "/DeviceCMYK"))
    {
        strcpy(context->state->stroke.currentColorSpace, data->data());
    }
    else
    {
        pdf_obj_get_colorspace(context->current_obj, data->data(), context->state->stroke.currentColorSpace);
    }
}

void handle_g(pdf_render* context)
{
    // for nonstroking
    auto data = context->deque->pop_front();
    float g = strtof(data->data(), NULL);
    // handle_G(context);
    //plutovg_canvas_set_rgb(context->canvas, g, g, g);
    context->state->fill.color[0] = g;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = g;
    strcpy(context->state->fill.currentColorSpace, "/DeviceGray");
}

void handle_G(pdf_render* context)
{
    // set both in one operation
    // gray
    auto data = context->deque->pop_front();
    float g = strtof(data->data(), NULL);
    context->state->stroke.color[0] = g;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = g;
    strcpy(context->state->stroke.currentColorSpace, "/DeviceGray");
}

void handle_k(pdf_render* context)
{
    // for nonstroking
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
    //plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fill.color[0] = r;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = b;
    strcpy(context->state->fill.currentColorSpace, "/DeviceCMYK");
}

void handle_K(pdf_render* context)
{
    // combine CS and SC for DeviceCMYK
    auto data = context->deque->pop_front();
    float k = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float y = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float m = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    float c = strtof(data4->data(), NULL);
    context->state->stroke.color[0] = (1.0 - c) * (1.0 - k);
    context->state->stroke.color[1] = (1.0 - m) * (1.0 - k);
    context->state->stroke.color[2] = (1.0 - y) * (1.0 - k);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceCMYK");
}

void handle_rg(pdf_render* context)
{
    // for nonstroking
    auto data = context->deque->pop_front();
    float b = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float g = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float r = strtof(data3->data(), NULL);
    //plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fill.color[0] = r;
    context->state->fill.color[1] = g;
    context->state->fill.color[2] = b;
    strcpy(context->state->fill.currentColorSpace, "/DeviceRGB");
    // handle_RG(context);
}


void handle_RG(pdf_render* context)
{
    // combine CS and SC for DeviceRGB
    auto data = context->deque->pop_front();
    float b = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float g = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float r = strtof(data3->data(), NULL);
    context->state->stroke.color[0] = r;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = b;
    // plutovg_canvas_set_rgb(context->canvas, gray, gray, gray);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceRGB");
}

void handle_sc(pdf_render* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);

    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray"))
    {
        // gray
        auto data = context->deque->pop_front();
        float g = strtof(data->data(), NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->state->fill.color[0] = g;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = g;
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        auto data = context->deque->pop_front();
        float b = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float g = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float r = strtof(data3->data(), NULL);
        // plutovg_canvas_set_rgb(context->canvas, r, g, b);
        context->state->fill.color[0] = r;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = b;
    }
    else if (!strcmp(context->state->fill.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
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

        context->state->fill.color[0] = r;
        context->state->fill.color[1] = g;
        context->state->fill.color[2] = b;
    }
}


void handle_SC(pdf_render* context)
{
    // set gray level, 0.0 to balck 1.0 to white
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray"))
    {
        // gray
        auto data = context->deque->pop_front();
        float g = strtof(data->data(), NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->state->stroke.color[0] = g;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = g;
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        auto data = context->deque->pop_front();
        float b = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float g = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float r = strtof(data3->data(), NULL);
        // plutovg_canvas_set_rgb(context->canvas, r, g, b);
        context->state->stroke.color[0] = r;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = b;
    }
    else if (!strcmp(context->state->stroke.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
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

        context->state->stroke.color[0] = r;
        context->state->stroke.color[1] = g;
        context->state->stroke.color[2] = b;
    }
}

void handle_scn(pdf_render* context)
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
        return handle_sc(context);
    }
    auto data = context->deque->pop_front();
    auto data2 = context->deque->pop_front();
    auto data3 = context->deque->pop_front();
}

void handle_SCN(pdf_render* context)
{
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_SC(context);
    }
  
    auto data = context->deque->pop_front();
    auto data2 = context->deque->pop_front();
    auto data3 = context->deque->pop_front();
}