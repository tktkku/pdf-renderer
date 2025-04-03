#include "render.h"
void handle_d1(pdf_context_t* context)
{
    // wx wy llx lly urx ury
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}
