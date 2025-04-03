#include "render.h"
void handle_i(pdf_context_t* context)
{
    // set flatness tolerance
    // flatness
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    //float i = strtof(buf, NULL);
    // context->graphics_state.flatness = i;
}
