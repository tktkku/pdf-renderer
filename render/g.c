#include "render.h"
void handle_g(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    // handle_G(context);
    plutovg_canvas_set_rgb(context->canvas, g, g, g);
    context->state->fillColor[0] = g;
    context->state->fillColor[1] = g;
    context->state->fillColor[2] = g;
}
