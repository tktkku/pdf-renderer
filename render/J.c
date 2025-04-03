#include "render.h"
void handle_J(pdf_context_t* context)
{
    // set cap style
    // lineCap
    pdf_stack_node_t node;
    char buf[1024] = { 0 };
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int c = atoi(buf);

    plutovg_canvas_set_line_cap(context->canvas, c);
}
