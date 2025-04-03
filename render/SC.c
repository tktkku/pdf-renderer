#include "render.h"
void handle_SC(pdf_context_t* context)
{
    // set gray level, 0.0 to balck 1.0 to white

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;

    if (!strcmp(context->state->currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->state->strokeColor[0] = g;
        context->state->strokeColor[1] = g;
        context->state->strokeColor[2] = g;
    }
    else if (!strcmp(context->state->currentColorSpace,
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
        context->state->strokeColor[0] = r;
        context->state->strokeColor[1] = g;
        context->state->strokeColor[2] = b;
    }
    else if (!strcmp(context->state->currentColorSpace,
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

        context->state->strokeColor[0] = r;
        context->state->strokeColor[1] = g;
        context->state->strokeColor[2] = b;
    }
}
