#include "render.h"
void handle_rg(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);
    plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->state->fillColor[0] = r;
    context->state->fillColor[1] = g;
    context->state->fillColor[2] = b;
    // handle_RG(context);
}
