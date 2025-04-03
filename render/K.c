#include "render.h"
void handle_K(pdf_context_t* context)
{
    // combine CS and SC for DeviceCMYK
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float k = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->state->strokeColor[0] = (1.0 - c) * (1.0 - k);
    context->state->strokeColor[1] = (1.0 - m) * (1.0 - k);
    context->state->strokeColor[2] = (1.0 - y) * (1.0 - k);
    strcpy(context->state->currentColorSpace, "/DeviceCMYK");
}
