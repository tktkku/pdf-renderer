#include "render.h"
void handle_c(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y1 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}
