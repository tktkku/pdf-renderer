#include "render.h"
void handle_TD(pdf_context_t* context)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ty = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float tx = strtof(buf, NULL);

    plutovg_canvas_translate(context->canvas, tx, ty);
    plutovg_canvas_move_to(context->canvas, 0, 0);
    context->state->textState.textLeading = -ty;
    context->state->textState.textLineWidth = 0;
    // side effect, set the leading parameter in the text state
    // -ty TL
    // tx ty Td
}
