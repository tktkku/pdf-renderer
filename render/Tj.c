#include "render.h"
void handle_Tj(pdf_context_t* context)
{
    // show / paint the glyphs for a string
    // string
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    if (context->state->textState.font == NULL)
        return;

    _do_text_render(context, node.data, node.size);
}