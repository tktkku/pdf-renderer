#include "render.h"
void handle_TL(pdf_context_t* context)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float t = strtof(buf, NULL);
    context->state->textState.textLeading = t;
}
