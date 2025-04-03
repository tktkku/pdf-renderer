#include "render.h"
void handle_cm(pdf_context_t* context)
{
    // change matrix CTM
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float a = strtof(buf, NULL);

    plutovg_matrix_t ctm;
    plutovg_matrix_init(&ctm, a, b, c, d, e, f);
    plutovg_canvas_transform(context->canvas, &ctm);
}
