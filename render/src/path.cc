#include "pdf-render.h"
#include "pdf-render-private.h"
#include "plutovg-private.h"
void stroke(pdf_render* context)
{
    //double r, g, b, a;
    plutovg_color_t color;
    color.r = context->canvas->state->color.r;
    color.g = context->canvas->state->color.g;
    color.b = context->canvas->state->color.b;
    color.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->stroke.color[0],
        context->state->stroke.color[1], context->state->stroke.color[2]);
    plutovg_canvas_stroke(context->canvas);
    plutovg_canvas_set_color(context->canvas, &color);
    // plutovg_canvas_set_rgb(context->canvas,
    //     context->fillColor[0],
    //     context->fillColor[1],
    //     context->fillColor[2]);
}


void handle_b_star(pdf_render* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B_star(pdf_render* context)
{
    // fill and then stroke the path, using the even-odd rule
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_b(pdf_render* context)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B(pdf_render* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_c(pdf_render* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    auto data = context->deque->pop_front();
    float y3 = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x3 = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float y2 = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    float x2 = strtof(data4->data(), NULL);
    auto data5 = context->deque->pop_front();
    float y1 = strtof(data5->data(), NULL);
    auto data6 = context->deque->pop_front();
    float x1 = strtof(data6->data(), NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}

void handle_F_f(pdf_render* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_f_star(pdf_render* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    plutovg_color_t c;
    c.r = context->canvas->state->color.r;
    c.g = context->canvas->state->color.g;
    c.b = context->canvas->state->color.b;
    c.a = context->canvas->state->color.a;
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_h(pdf_render* context)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    plutovg_canvas_close_path(context->canvas);
}

void handle_l(pdf_render* context)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    auto data = context->deque->pop_front();
    float y = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x = strtof(data2->data(), NULL);
    plutovg_canvas_line_to(context->canvas, x, y);
}

void handle_m(pdf_render* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    auto data = context->deque->pop_front();
    float y = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x = strtof(data2->data(), NULL);
    plutovg_canvas_move_to(context->canvas, x, y);
}

void handle_n(pdf_render* context)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_new_path(context->canvas);
}

void handle_re(pdf_render* context)
{
    // append a rectangle to the current path as a complete subpath
    // lower-left corner (x, y)
    // x y width height

    /*
    the operation
    x y width height re
    is equivalent to
    x y m
    (x+width) y l
    (x+width) (y+height) l
    x (y+height) l
    h
    */

    auto data = context->deque->pop_front();
    float height = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float width = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float y = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    float x = strtof(data4->data(), NULL);

    plutovg_canvas_rect(context->canvas, x, y, width, height);
}

void handle_s(pdf_render* context)
{
    // close and stroke the path
    // same as the sequence h S
    //
    plutovg_canvas_close_path(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_S(pdf_render* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_v(pdf_render* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    float x1, y1, x2, y2, x3, y3;
    auto data = context->deque->pop_front();
    y3 = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    x3 = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    y2 = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    x2 = strtof(data4->data(), NULL);

    plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}
void handle_W_star(pdf_render* context)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule

    plutovg_canvas_clip(context->canvas);
}

void handle_w(pdf_render* context)
{
    // set line width
    // lineWidth
    auto data = context->deque->pop_front();
    float w = strtof(data->data(), NULL);
    plutovg_canvas_set_line_width(context->canvas, w);
    context->state->lineWidth = w;
}

void handle_W(pdf_render* context)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule

    plutovg_canvas_clip(context->canvas);
}

void handle_y(pdf_render* context)
{
    // append a cubic Bezier curve to the current path
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x3, y3) as the Bezier control points
    // x2 y2 same as x3 y3
    // x1 y1 x3 y3
    auto data = context->deque->pop_front();
    float y3 = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x3 = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    float y1 = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    float x1 = strtof(data4->data(), NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x3, y3, x3, y3);
}
