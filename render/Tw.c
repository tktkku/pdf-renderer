#include "render.h"
void handle_Tw(pdf_context_t* context)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    context->state->textState.wordSpacing = w;
}
