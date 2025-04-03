#include "render.h"
void handle_Tr(pdf_context_t* context)
{
    // set text rendering mode
    // mode initial value =0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int v = strtof(buf, NULL);
    // STROKE FILL BOTH CLIP

    int mode = 0;
    if ((v & 0x1) == 0)
    {
        mode |= 2;
    }
    if ((v & 0x4) != 0)
    {
        mode |= 4;
    }
    if (((v & 0x1) ^ ((v & 0x2) >> 1)) != 0)
    {
        mode |= 1;
    }
    context->state->textState.textMode = mode;
}
