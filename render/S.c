#include "render.h"
void handle_S(pdf_context_t* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}
