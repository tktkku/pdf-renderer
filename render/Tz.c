#include "render.h"
void handle_Tz(pdf_context_t* context)
{
    // horizontal scaling
    // scale initial value=100
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float h = strtof(buf, NULL);
    //plutovg_canvas_scale(context->canvas, h / 100.0, 1.0);
    context->state->textState.horizontalScaling = h;
}
