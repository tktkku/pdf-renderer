#include "render.h"
void handle_v(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float x1, y1, x2, y2, x3, y3;
    pdf_stack_pop(context->stack, &node);
    y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x2 = strtof(buf, NULL);

    plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}
