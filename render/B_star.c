#include "render.h"
void handle_B_star(pdf_context_t* context)
{
    // fill and then stroke the path, using the even-odd rule
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}
