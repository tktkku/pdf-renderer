#include "render.h"
void handle_B(pdf_context_t* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}