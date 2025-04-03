#include "render.h"
void handle_cs(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // stack_node_t node;
    // node.data = buf;
    // stack_pop(context->stack, &node);

    handle_CS(context);
}
