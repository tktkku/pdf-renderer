#include "render.h"
void handle_Tc(pdf_context_t* context)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->state->textState.characterSpacing = c;
}
