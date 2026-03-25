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

void fill(pdf_render* context)
{
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

void _do_path(pdf_render* context, int type)
{
    if (type & PDF_OPERATION_PATH_EVEN_ODD)
    {
        plutovg_canvas_set_fill_rule(context->canvas,
                    PLUTOVG_FILL_RULE_EVEN_ODD);
    }
    if (type & PDF_OPERATION_PATH_NON_ZERO)
    {
        plutovg_canvas_set_fill_rule(context->canvas,
                    PLUTOVG_FILL_RULE_NON_ZERO);
    }
    if (type & PDF_OPERATION_PATH_CLOSE)
    {
        plutovg_canvas_close_path(context->canvas);
    }
    if (type & PDF_OPERATION_PATH_NEW_PATH)
    {
        plutovg_canvas_new_path(context->canvas);
    }
    if (type & PDF_OPERATION_PATH_FILL)
    {
        fill(context);
    }
    if (type & PDF_OPERATION_PATH_STROKE)
    {
        stroke(context);
    }
}

std::function<void()> handle_b_star(pdf_render* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
                PDF_OPERATION_PATH_CLOSE | 
                PDF_OPERATION_PATH_FILL | 
                PDF_OPERATION_PATH_STROKE);
    };
}


std::function<void()> handle_B_star(pdf_render* context)
{
    // fill and then stroke the path, using the even-odd rule
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

std::function<void()> handle_b(pdf_render* context)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_CLOSE |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

std::function<void()> handle_B(pdf_render* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

std::function<void()> handle_c(pdf_render* context)
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
    return [context, x1, y1, x2, y2, x3, y3] {
        plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
    };
}

std::function<void()> handle_F_f(pdf_render* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_FILL);
    };
}

std::function<void()> handle_f_star(pdf_render* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_FILL); 
    };
}

std::function<void()> handle_h(pdf_render* context)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    return [context] { _do_path(context, PDF_OPERATION_PATH_CLOSE); };
}

std::function<void()> handle_l(pdf_render* context)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    auto data = context->deque->pop_front();
    float y = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x = strtof(data2->data(), NULL);
    return [context, x, y] {
        plutovg_canvas_line_to(context->canvas, x, y);
    }; 
}

std::function<void()> handle_m(pdf_render* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    auto data = context->deque->pop_front();
    float y = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float x = strtof(data2->data(), NULL);
    return [context, x, y] {
        plutovg_canvas_move_to(context->canvas, x, y);
    };
}

std::function<void()> handle_n(pdf_render* context)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    return [context] { _do_path(context,  PDF_OPERATION_PATH_NEW_PATH); };
}

std::function<void()> handle_re(pdf_render* context)
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

    return [context, x, y, width, height] {
        plutovg_canvas_rect(context->canvas, x, y, width, height);
    };
}

std::function<void()> handle_s(pdf_render* context)
{
    // close and stroke the path
    // same as the sequence h S
    //
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_CLOSE |
        PDF_OPERATION_PATH_STROKE); 
    };
}

std::function<void()> handle_S(pdf_render* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    return [context] { _do_path(context, PDF_OPERATION_PATH_STROKE); };
}

std::function<void()> handle_v(pdf_render* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    float x2, y2, x3, y3;
    auto data = context->deque->pop_front();
    y3 = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    x3 = strtof(data2->data(), NULL);
    auto data3 = context->deque->pop_front();
    y2 = strtof(data3->data(), NULL);
    auto data4 = context->deque->pop_front();
    x2 = strtof(data4->data(), NULL);

    return [context, x2, y2, x3, y3] {
        float x1, y1;
        plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
        plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
    };
}
std::function<void()> handle_W_star(pdf_render* context)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_CLIP); 
    };
}

std::function<void()> handle_w(pdf_render* context)
{
    // set line width
    // lineWidth
    auto data = context->deque->pop_front();
    float w = strtof(data->data(), NULL);
    return [context, w] {
        plutovg_canvas_set_line_width(context->canvas, w);
        context->state->lineWidth = w;
    };

}

std::function<void()> handle_W(pdf_render* context)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule
    return [context] {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_CLIP); 
    };
}

std::function<void()> handle_y(pdf_render* context)
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
    return [context, x1, y1, x3, y3] {
        plutovg_canvas_cubic_to(context->canvas, x1, y1, x3, y3, x3, y3);
    };
}
