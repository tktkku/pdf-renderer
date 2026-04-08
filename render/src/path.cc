#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
void stroke(pdf_render* context)
{
    //double r, g, b, a;
    float r, g, b, a;
    PDF_RENDERER_CALL(context->renderer, get_color, &r, &g, &b);
    PDF_RENDERER_CALL(context->renderer, set_color, context->state->stroke.color[0],
        context->state->stroke.color[1], context->state->stroke.color[2]);
    PDF_RENDERER_CALL(context->renderer, stroke);
    PDF_RENDERER_CALL(context->renderer, set_color, r, g, b);
}

void fill(pdf_render* context)
{
    float r, g, b, a;
    PDF_RENDERER_CALL(context->renderer, get_color, &r, &g, &b);
    PDF_RENDERER_CALL(context->renderer, set_color, context->state->fill.color[0],
        context->state->fill.color[1], context->state->fill.color[2]);
    PDF_RENDERER_CALL(context->renderer, fill);
    PDF_RENDERER_CALL(context->renderer, set_color, r, g, b);
}

void do_path(pdf_render* context, int type)
{
    if (type & PDF_OPERATION_PATH_EVEN_ODD)
    {
        PDF_RENDERER_CALL(context->renderer, set_fill_rule, true);
    }
    if (type & PDF_OPERATION_PATH_NON_ZERO)
    {
        PDF_RENDERER_CALL(context->renderer, set_fill_rule, false);
    }
    if (type & PDF_OPERATION_PATH_CLOSE)
    {
        PDF_RENDERER_CALL(context->renderer, close_path);
    }
    if (type & PDF_OPERATION_PATH_NEW_PATH)
    {
        PDF_RENDERER_CALL(context->renderer, new_path);
    }
    if (type & PDF_OPERATION_PATH_CLIP)
    {
        PDF_RENDERER_CALL(context->renderer, clip);
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
        do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
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
        do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
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
        do_path(context, PDF_OPERATION_PATH_NON_ZERO |
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
        do_path(context, PDF_OPERATION_PATH_NON_ZERO |
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
        cmd->matrix.a = x1;
        cmd->matrix.b = y1;
        cmd->matrix.c = x2;
        cmd->matrix.d = y2;
        cmd->matrix.e = x3;
        cmd->matrix.f = y3;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, cubic_to, 
            cmd->matrix.a, cmd->matrix.b, 
            cmd->matrix.c, cmd->matrix.d, 
            cmd->matrix.e, cmd->matrix.f);
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
        do_path(context, PDF_OPERATION_PATH_NON_ZERO |
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
        do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
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
    { do_path(context, PDF_OPERATION_PATH_CLOSE); };
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
        cmd->matrix.a = x;
        cmd->matrix.b = y;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, line_to, cmd->matrix.a, cmd->matrix.b);
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
        cmd->matrix.a = x;
        cmd->matrix.b = y;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, move_to, cmd->matrix.a, cmd->matrix.b);
    }
}

void handle_n(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // end the path object without filling or stroking it
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_n;
    }
    else{ do_path(context,  PDF_OPERATION_PATH_NEW_PATH); };
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
        cmd->matrix.a = x;
        cmd->matrix.b = y;
        cmd->matrix.c = width;
        cmd->matrix.d = height;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, rect, 
            cmd->matrix.a, cmd->matrix.b, cmd->matrix.c, cmd->matrix.d);
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
        do_path(context, PDF_OPERATION_PATH_CLOSE |
        PDF_OPERATION_PATH_STROKE); 
    };
}

void handle_S(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // stroke the path
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_S;
    }
    else { do_path(context, PDF_OPERATION_PATH_STROKE); };
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
        cmd->matrix.a = x2;
        cmd->matrix.b = y2;
        cmd->matrix.c = x3;
        cmd->matrix.d = y3;
    }
    else
    {
        float x1, y1;
        PDF_RENDERER_CALL(context->renderer, get_current_point, &x1, &y1);
        PDF_RENDERER_CALL(context->renderer, cubic_to, 
            x1, y1, cmd->matrix.a, cmd->matrix.b, 
            cmd->matrix.c, cmd->matrix.d);
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
        do_path(context, PDF_OPERATION_PATH_EVEN_ODD |
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
        cmd->floatVal = w;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, set_line_width, cmd->floatVal);
        context->state->lineWidth = cmd->floatVal;
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
        do_path(context, PDF_OPERATION_PATH_NON_ZERO |
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
        cmd->matrix.a = x1;
        cmd->matrix.b = y1;
        cmd->matrix.c = x3;
        cmd->matrix.d = y3;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, cubic_to, 
            cmd->matrix.a, cmd->matrix.b, 
            cmd->matrix.c, cmd->matrix.d,
            cmd->matrix.c, cmd->matrix.d);
    }
}

void handle_BDC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag properties
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        if (!strcmp(data->data(), ">>"))
        {
            context->deque->pop_front(); // str
            context->deque->pop_front(); // name
            context->deque->pop_front(); // <<
        }
        else
        {
            context->deque->pop_front(); // name
        }
        context->deque->pop_front(); // tag
        cmd->type = TOKEN_OPERATOR_BDC;
    }
}
void handle_BI(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // begin an inline image object
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        char** pptr = ((char**)(void*)data->data());
        char* buffer = *pptr;

        auto data1 = context->deque->pop_front();
        int* ptr1 = ((int*)(void*)data1->data());
        int width = *ptr1;
        
        auto data2 = context->deque->pop_front();
        int* ptr2 = ((int*)(void*)data2->data());
        int height = *ptr2;

        auto data3 = context->deque->pop_front();
        int* ptr3 = ((int*)(void*)data3->data());
        int channels = *ptr3;

        auto data4 = context->deque->pop_front();
        int* ptr4 = ((int*)(void*)data4->data());
        int size = *ptr4;
  
        cmd->type = TOKEN_OPERATOR_Do;
        cmd->Do.type = XOBJ_IMAGE;
        cmd->Do.width = width;
        cmd->Do.height = height;
        cmd->Do.channels = channels;
        cmd->Do.pixels = (unsigned char*)buffer;
        cmd->Do.pixels_size = size;
    }
}

void handle_BMC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag
    if (dry_run)
    {
        context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_BMC;
    }
}


void handle_cm(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // change matrix CTM
    // a b c d e f
    if (dry_run)
    {
        char buf[1024] = { 0 };
        auto data = context->deque->pop_front();
        float f = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float e = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float d = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float c = strtof(data4->data(), NULL);
        auto data5 = context->deque->pop_front();
        float b = strtof(data5->data(), NULL);
        auto data6 = context->deque->pop_front();
        float a = strtof(data6->data(), NULL);
        cmd->type = TOKEN_OPERATOR_cm;
        cmd->matrix.a = a;
        cmd->matrix.b = b;
        cmd->matrix.c = c;
        cmd->matrix.d = d;
        cmd->matrix.e = e;
        cmd->matrix.f = f;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, transform, 
            cmd->matrix.a, cmd->matrix.b, cmd->matrix.c,
            cmd->matrix.d, cmd->matrix.e, cmd->matrix.f);
    }
}

void handle_d(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set line dash pattern
    // dashArray dashPhase
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float offset = strtof(data->data(), NULL);

        int index = 2;
        float dashs[2] = { 0 };
        while (true)
        {
            auto data2 = context->deque->pop_front();
            if (!strcmp(data2->data(), "]"))
            {
                // ignore
            }
            else if (!strcmp(data2->data(), "["))
            {
                break;
            }
            else
            {
                float dash = strtof(data2->data(), NULL);
                if (index > 0)
                    dashs[--index] = dash;
            }
        }
        cmd->type = TOKEN_OPERATOR_d;
        cmd->d.offset = offset;
        cmd->d.dashs[0] = dashs[0];
        cmd->d.dashs[1] = dashs[1];
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, set_dash_offset, cmd->d.offset);
        PDF_RENDERER_CALL(context->renderer, set_dash_array, cmd->d.dashs, 2);
    }
}

void handle_d0(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // wx wy
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float wy = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float wx = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_d0;
    }
}

void handle_d1(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // wx wy llx lly urx ury
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float ury = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float urx = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        float lly = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        float llx = strtof(data4->data(), NULL);
        auto data5 = context->deque->pop_front();
        float wy = strtof(data5->data(), NULL);
        auto data6 = context->deque->pop_front();
        float wx = strtof(data6->data(), NULL);
        cmd->type = TOKEN_OPERATOR_d1;
    }
}

void handle_DP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag properties
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        auto data2 = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_DP;
    }
}
// void handle_EI(pdf_render* context, pdf_render_command* cmd, bool dry_run)
// {
//     // end an inline image object
// }

void handle_EMC(pdf_render* context, pdf_render_command* cmd, bool dry_run) {
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_EMC;
    }
}

void handle_gs(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set specified parameters
    // dictName shall be the name of
    // a graphics state parameter dictionary
    // in the ExtGState subdictionary of the current resource dictionary
    // dictName
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_gs;
    }
}

void handle_i(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set flatness tolerance
    // flatness
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_i;
    }
}

// void handle_ID(pdf_render* context, pdf_render_command* cmd, bool dry_run)
// {
//     // begin the image data for an inline image object
// }

void handle_j(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set join style
    // lineJoin
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int j = atoi(data->data());
        cmd->type = TOKEN_OPERATOR_j;
        cmd->intVal = j;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, set_line_join, cmd->intVal);
        context->state->lineJoin = cmd->intVal;
    }
}

void handle_J(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set cap style
    // lineCap
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int c = atoi(data->data());
        cmd->type = TOKEN_OPERATOR_J;
        cmd->intVal = c;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, set_line_cap, cmd->intVal);
        context->state->lineCap = cmd->intVal;
    }
}


void handle_M(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set miter limit
    // miterLimit
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float m = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_M;
        cmd->floatVal = m;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, set_miter_limit, cmd->floatVal);
        context->state->miterLimit = cmd->floatVal;
    }
}
void handle_MP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // tag
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_MP;
    }
}

void handle_q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // store state
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_q;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, save);
        if (context->state == NULL) _init_state(context);
        pdf_graphics_state_t* new_state = (pdf_graphics_state_t*)malloc(sizeof(pdf_graphics_state_t));
        memcpy(new_state, context->state, sizeof(pdf_graphics_state_t));
        new_state->next = context->state;
        context->state = new_state;
    }
}

void handle_Q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // restore state
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_Q;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, restore);
        pdf_graphics_state_t* old_state = context->state;
        context->state = old_state->next;
        free(old_state);
        if (context->state == NULL) _init_state(context);
        PDF_RENDERER_CALL(context->renderer, set_line_width, context->state->lineWidth);
        PDF_RENDERER_CALL(context->renderer, set_line_cap, context->state->lineCap);
        PDF_RENDERER_CALL(context->renderer, set_line_join, context->state->lineJoin);
        PDF_RENDERER_CALL(context->renderer, set_miter_limit, context->state->miterLimit);
    }
}


void handle_ri(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set color rendering intent
    // intent
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_ri;
    }
}

void handle_sh(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // name
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_sh;
    }
}