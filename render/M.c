#include "render.h"
void handle_M(pdf_context_t* context)
{
    // set miter limit
    // miterLimit
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    plutovg_canvas_set_miter_limit(context->canvas, m);
}
