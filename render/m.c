#include "render.h"
void handle_m(pdf_context_t* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_move_to(context->canvas, x, y);
}
