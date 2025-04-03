#include "render.h"
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
    plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fillColor[0] = r;
    context->state->fillColor[1] = g;
    context->state->fillColor[2] = b;
}
