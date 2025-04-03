#include "render.h"
void handle_q(pdf_context_t* context)
{
    // store state
    plutovg_canvas_save(context->canvas);
    pdf_graphics_state_t* new_state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
    memcpy(new_state, context->state, sizeof(pdf_graphics_state_t));
    new_state->next = context->state;
    context->state = new_state;
}
