#include "render.h"
void handle_Ts(pdf_context_t* context)
{
    // set text rise
    // rise initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);

    context->state->textState.textRise = r;
}
