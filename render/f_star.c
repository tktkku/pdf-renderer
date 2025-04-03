#include "render.h"
void handle_f_star(pdf_context_t* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    plutovg_canvas_fill(context->canvas);
}
