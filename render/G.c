#include "render.h"
void handle_G(pdf_context_t* context)
{
    // set both in one operation
    // gray
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    context->state->strokeColor[0] = g;
    context->state->strokeColor[1] = g;
    context->state->strokeColor[2] = g;
    strcpy(context->state->currentColorSpace, "/DeviceGray");
}
