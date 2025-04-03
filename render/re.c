#include "render.h"
void handle_re(pdf_context_t* context)
{
    // append a rectangle to the current path as a complete subpath
    // lower-left corner (x, y)
    // x y width height

    /*
    the operation
    x y width height re
    is equivalent to
    x y m
    (x+width) y l
    (x+width) (y+height) l
    x (y+height) l
    h
    */

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float height = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float width = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);

    plutovg_canvas_rect(context->canvas, x, y, width, height);
}
