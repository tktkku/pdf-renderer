#include "render.h"
void handle_F_f(pdf_context_t* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill

    plutovg_canvas_set_rgb(context->canvas, context->state->fillColor[0],
        context->state->fillColor[1], context->state->fillColor[2]);
    plutovg_canvas_fill(context->canvas);
}
