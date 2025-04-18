#include "render.h"
#include "pdf-private.h"
void handle_cs(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);

    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    if (!strcmp(buf, "/DeviceGray") || !strcmp(buf, "/DeviceRGB") || !strcmp(buf, "/DeviceCMYK"))
    {
        strcpy(context->state->stroke.currentColorSpace, buf);
    }
    else
    {
        pdf_obj_get_colorspace(context->current_obj, buf, context->state->stroke.currentColorSpace);
    }
}

void handle_g(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float g = strtof(buf, NULL);
    context->state->stroke.color[0] = g;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = g;
    strcpy(context->state->stroke.currentColorSpace, "/DeviceGray");
}

void handle_k(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float k = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float m = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float k = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float m = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float c = strtof(buf, NULL);
    context->state->stroke.color[0] = (1.0 - c) * (1.0 - k);
    context->state->stroke.color[1] = (1.0 - m) * (1.0 - k);
    context->state->stroke.color[2] = (1.0 - y) * (1.0 - k);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceCMYK");
}

void handle_rg(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float b = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float g = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float b = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float g = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float r = strtof(buf, NULL);
    context->state->stroke.color[0] = r;
    context->state->stroke.color[1] = g;
    context->state->stroke.color[2] = b;
    // plutovg_canvas_set_rgb(context->canvas, gray, gray, gray);
    strcpy(context->state->stroke.currentColorSpace, "/DeviceRGB");
}

void handle_sc(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // deque_node_t node;
    // node.data = buf;
    // deque_pop(context->deque, &node);
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;

    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_deque_pop_front(context->deque, &node);
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

        pdf_deque_pop_front(context->deque, &node);
        float b = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float g = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
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
        pdf_deque_pop_front(context->deque, &node);
        float k = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float y = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float m = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;

    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_deque_pop_front(context->deque, &node);
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

        pdf_deque_pop_front(context->deque, &node);
        float b = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float g = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
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
        pdf_deque_pop_front(context->deque, &node);
        float k = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float y = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
        float m = strtof(buf, NULL);
        pdf_deque_pop_front(context->deque, &node);
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
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
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
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
}