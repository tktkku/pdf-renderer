#include "pdf-private.h"
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

void handle_b_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_b_star;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
                PDF_OPERATION_PATH_CLOSE | 
                PDF_OPERATION_PATH_FILL | 
                PDF_OPERATION_PATH_STROKE);
    }
}


void handle_B_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // fill and then stroke the path, using the even-odd rule
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_B_star;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

void handle_b(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_b;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_CLOSE |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

void handle_B(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // fill and then stroke the path, using the nonzero winding number rule
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_B;
    }
    else 
    {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_FILL |
        PDF_OPERATION_PATH_STROKE);
    };
}

void handle_c(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    if (dry_run)
    {
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
        cmd->type = TOKEN_OPERATOR_c;
        cmd->c.x1 = x1;
        cmd->c.x2 = x2;
        cmd->c.x3 = x3;
        cmd->c.y1 = y1;
        cmd->c.y2 = y2;
        cmd->c.y3 = y3;
    }
    else
    {
        plutovg_canvas_cubic_to(context->canvas, 
            cmd->c.x1, cmd->c.y1, cmd->c.x2, cmd->c.y2, cmd->c.x3, cmd->c.y3);
    }
}

void handle_F_f(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_F;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_FILL);
    };
}

void handle_f_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_f_star;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_FILL); 
    };
}

void handle_h(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_h;
    }
    else 
    { _do_path(context, PDF_OPERATION_PATH_CLOSE); };
}

void handle_l(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float y = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float x = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_l;
        cmd->l.x = x;
        cmd->l.y = y;
    }
    else
    {
        plutovg_canvas_line_to(context->canvas, cmd->l.x, cmd->l.y);
    }
}

void handle_m(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float y = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float x = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_m;
        cmd->m.x = x;
        cmd->m.y = y;
    }
    else
    {
        plutovg_canvas_move_to(context->canvas, cmd->m.x, cmd->m.y);
    }
}

void handle_n(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_n;
    }
    else{ _do_path(context,  PDF_OPERATION_PATH_NEW_PATH); };
}

void handle_re(pdf_render* context, pdf_render_command* cmd, bool dry_run)
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
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float height = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float width = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float y = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float x = strtof(data4->data(), NULL);
        cmd->type = TOKEN_OPERATOR_re;
        cmd->re.x = x;
        cmd->re.y = y;
        cmd->re.width = width;
        cmd->re.height = height;
    }
    else
    {
        plutovg_canvas_rect(context->canvas, 
            cmd->re.x, cmd->re.y, cmd->re.width, cmd->re.height);
    }
}

void handle_s(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // close and stroke the path
    // same as the sequence h S
    //
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_s;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_CLOSE |
        PDF_OPERATION_PATH_STROKE); 
    };
}

void handle_S(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_S;
    }
    else { _do_path(context, PDF_OPERATION_PATH_STROKE); };
}

void handle_v(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    if (dry_run)
    {
        float x2, y2, x3, y3;
        auto data = context->deque->pop_front();
        y3 = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        x3 = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        y2 = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        x2 = strtof(data4->data(), NULL);
        cmd->type = TOKEN_OPERATOR_v;
        cmd->v.x2 = x2;
        cmd->v.y2 = y2;
        cmd->v.x3 = x3;
        cmd->v.y3 = y3;
    }
    else
    {
        float x1, y1;
        plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
        plutovg_canvas_cubic_to(context->canvas, 
            x1, y1, cmd->v.x2, cmd->v.y2, cmd->v.x3, cmd->v.y3);
    }
}
void handle_W_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_W_star;
    }
    else {
        _do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
        PDF_OPERATION_PATH_CLIP); 
    };
}

void handle_w(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set line width
    // lineWidth
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float w = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_w;
        cmd->w.f = w;
    }
    else
    {
        plutovg_canvas_set_line_width(context->canvas, cmd->w.f);
        context->state->lineWidth = cmd->w.f;
    }
}

void handle_W(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_W;
    }
    else
    {
        _do_path(context, PDF_OPERATION_PATH_NON_ZERO |
        PDF_OPERATION_PATH_CLIP); 
    };
}

void handle_y(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // append a cubic Bezier curve to the current path
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x3, y3) as the Bezier control points
    // x2 y2 same as x3 y3
    // x1 y1 x3 y3
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float y3 = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float x3 = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float y1 = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float x1 = strtof(data4->data(), NULL);
        cmd->type = TOKEN_OPERATOR_y;
        cmd->y.x1 = x1;
        cmd->y.y1 = y1;
        cmd->y.x3 = x3;
        cmd->y.y3 = y3;
    }
    else
    {
        plutovg_canvas_cubic_to(context->canvas, 
            cmd->y.x1, cmd->y.y1, cmd->y.x3, cmd->y.y3, cmd->y.x3, cmd->y.y3);
    }
}
