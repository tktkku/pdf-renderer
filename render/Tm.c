#include "render.h"
void handle_Tm(pdf_context_t* context)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float a, b, c, d, e, f;
    pdf_stack_pop(context->stack, &node);
    f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    a = strtof(buf, NULL);

    plutovg_matrix_t m;
    plutovg_matrix_init(&m, a, b, c, d, e, f);

    //plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
    // set font matrix
    plutovg_canvas_transform(context->canvas, &m);
    plutovg_canvas_move_to(context->canvas, 0, 0);
}
