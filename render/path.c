#include "render.h"
#include "pdf-private.h"

void stroke(pdf_context_t* context)
{
    //double r, g, b, a;
    plutovg_color_t color;
    plutovg_canvas_get_color(context->canvas, &color);
    plutovg_canvas_set_rgb(context->canvas, context->state->stroke.color[0],
        context->state->stroke.color[1], context->state->stroke.color[2]);
    plutovg_canvas_stroke(context->canvas);
    plutovg_canvas_set_color(context->canvas, &color);
    // plutovg_canvas_set_rgb(context->canvas,
    //     context->fillColor[0],
    //     context->fillColor[1],
    //     context->fillColor[2]);
}


void handle_b_star(pdf_context_t* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B_star(pdf_context_t* context)
{
    // fill and then stroke the path, using the even-odd rule
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_b(pdf_context_t* context)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    plutovg_canvas_close_path(context->canvas);
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B(pdf_context_t* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_c(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float y3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y2 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x2 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y1 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}

void handle_F_f(pdf_context_t* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_f_star(pdf_context_t* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    plutovg_color_t c;
    plutovg_canvas_get_color(context->canvas, &c);
    plutovg_canvas_set_rgb(context->canvas, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    plutovg_canvas_fill(context->canvas);
    plutovg_canvas_set_color(context->canvas, &c);
}

void handle_h(pdf_context_t* context)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    plutovg_canvas_close_path(context->canvas);
}

void handle_l(pdf_context_t* context)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float y = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_line_to(context->canvas, x, y);
}

void handle_m(pdf_context_t* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float y = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_move_to(context->canvas, x, y);
}

void handle_n(pdf_context_t* context)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_new_path(context->canvas);
}

void handle_re(pdf_context_t* context)
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

    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float height = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float width = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x = strtof(buf, NULL);

    plutovg_canvas_rect(context->canvas, x, y, width, height);
}

void handle_s(pdf_context_t* context)
{
    // close and stroke the path
    // same as the sequence h S
    //
    plutovg_canvas_close_path(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_S(pdf_context_t* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_v(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    float x1, y1, x2, y2, x3, y3;
    pdf_deque_pop_front(context->deque, &node);
    y3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    x3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    y2 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    x2 = strtof(buf, NULL);

    plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}
void handle_W_star(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule

    plutovg_canvas_clip(context->canvas);
}

void handle_w(pdf_context_t* context)
{
    // set line width
    // lineWidth
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float w = strtof(buf, NULL);
    plutovg_canvas_set_line_width(context->canvas, w);
    context->state->lineWidth = w;
}

void handle_W(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule

    plutovg_canvas_clip(context->canvas);
}

void handle_y(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current path
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x3, y3) as the Bezier control points
    // x2 y2 same as x3 y3
    // x1 y1 x3 y3
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float y3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x3 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float y1 = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x3, y3, x3, y3);
}
