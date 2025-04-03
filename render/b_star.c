#include "render.h"
void handle_b_star(pdf_context_t* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}
