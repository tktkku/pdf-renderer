#include "render.h"
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
    context->state->strokeColor[0] = r;
    context->state->strokeColor[1] = g;
    context->state->strokeColor[2] = b;
    // plutovg_canvas_set_rgb(context->canvas, gray, gray, gray);
    strcpy(context->state->currentColorSpace, "/DeviceRGB");
}
