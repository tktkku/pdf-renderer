#include "render.h"
void handle_T_star(pdf_context_t* context)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    plutovg_canvas_translate(context->canvas, 0, -context->state->textState.textLeading);
    plutovg_canvas_move_to(context->canvas, 0, 0);
    context->state->textState.textLineWidth = 0;
}
