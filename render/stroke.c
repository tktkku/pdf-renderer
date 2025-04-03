#include "render.h"
void stroke(pdf_context_t* context)
{
    //double r, g, b, a;
    plutovg_color_t color;
    plutovg_canvas_get_color(context->canvas, &color);
    plutovg_canvas_set_rgb(context->canvas, context->state->strokeColor[0],
        context->state->strokeColor[1], context->state->strokeColor[2]);
    plutovg_canvas_stroke(context->canvas);
    plutovg_canvas_set_color(context->canvas, &color);
    // plutovg_canvas_set_rgb(context->canvas,
    //     context->fillColor[0],
    //     context->fillColor[1],
    //     context->fillColor[2]);
}
