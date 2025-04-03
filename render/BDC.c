#include "render.h"
void handle_BDC(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}