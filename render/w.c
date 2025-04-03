#include "render.h"
void handle_w(pdf_context_t* context)
{
    // set line width
    // lineWidth
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    plutovg_canvas_set_line_width(context->canvas, w);
}
