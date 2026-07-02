#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
// ponytail: pop helpers to eliminate ~200 lines of duplicate pop+strtof boilerplate
static inline float pop_float(pdf_deque* deque) {
    auto data = deque->pop_front();
    return strtof(data->data(), NULL);
}
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
    if (dry_run)
    {
        float y3 = pop_float(context->deque), x3 = pop_float(context->deque);
        float y2 = pop_float(context->deque), x2 = pop_float(context->deque);
        float y1 = pop_float(context->deque), x1 = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_c;
        cmd->matrix.a = x1; cmd->matrix.b = y1;
        cmd->matrix.c = x2; cmd->matrix.d = y2;
        cmd->matrix.e = x3; cmd->matrix.f = y3;
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

void handle_n(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // end the path object without filling or stroking it
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_n;
    }
    else{ do_path(context,  PDF_OPERATION_PATH_NEW_PATH); };
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
    if (dry_run)
    {
        float y3 = pop_float(context->deque), x3 = pop_float(context->deque);
        float y2 = pop_float(context->deque), x2 = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_v;
        cmd->matrix.a = x2; cmd->matrix.b = y2;
        cmd->matrix.c = x3; cmd->matrix.d = y3;
    }
    else
    {
        float x1, y1;
        PDF_RENDERER_CALL(context->renderer, get_current_point, &x1, &y1);
        PDF_RENDERER_CALL(context->renderer, cubic_to, x1, y1, cmd->matrix.a, cmd->matrix.b, cmd->matrix.c, cmd->matrix.d);
    }
}

void handle_y(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        float y3 = pop_float(context->deque), x3 = pop_float(context->deque);
        float y1 = pop_float(context->deque), x1 = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_y;
        cmd->matrix.a = x1; cmd->matrix.b = y1;
        cmd->matrix.c = x3; cmd->matrix.d = y3;
    }
    else
    {
        PDF_RENDERER_CALL(context->renderer, cubic_to, cmd->matrix.a, cmd->matrix.b, cmd->matrix.c, cmd->matrix.d, cmd->matrix.c, cmd->matrix.d);
    }
}

void handle_m(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        float y = pop_float(context->deque), x = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_m;
        cmd->matrix.a = x; cmd->matrix.b = y;
    }
    else { PDF_RENDERER_CALL(context->renderer, move_to, cmd->matrix.a, cmd->matrix.b); }
}

void handle_l(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        float y = pop_float(context->deque), x = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_l;
        cmd->matrix.a = x; cmd->matrix.b = y;
    }
    else { PDF_RENDERER_CALL(context->renderer, line_to, cmd->matrix.a, cmd->matrix.b); }
}

void handle_re(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        float h = pop_float(context->deque), w = pop_float(context->deque);
        float y = pop_float(context->deque), x = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_re;
        cmd->matrix.a = x; cmd->matrix.b = y;
        cmd->matrix.c = w; cmd->matrix.d = h;
    }
    else { PDF_RENDERER_CALL(context->renderer, rect, cmd->matrix.a, cmd->matrix.b, cmd->matrix.c, cmd->matrix.d); }
}

void handle_cm(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        float f = pop_float(context->deque), e = pop_float(context->deque);
        float d = pop_float(context->deque), c = pop_float(context->deque);
        float b = pop_float(context->deque), a = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_cm;
        cmd->matrix.a = a; cmd->matrix.b = b;
        cmd->matrix.c = c; cmd->matrix.d = d;
        cmd->matrix.e = e; cmd->matrix.f = f;
    }
    else { PDF_RENDERER_CALL(context->renderer, transform, cmd->matrix.a, cmd->matrix.b, cmd->matrix.c, cmd->matrix.d, cmd->matrix.e, cmd->matrix.f); }
}

void handle_w(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        cmd->floatVal = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_w;
    } else {
        PDF_RENDERER_CALL(context->renderer, set_line_width, cmd->floatVal);
        context->state->lineWidth = cmd->floatVal;
    }
}

void handle_d(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        float offset = pop_float(context->deque);
        float dashs[2] = {0, 0}; int idx = 2;
        while (true) {
            auto d2 = context->deque->pop_front();
            if (!strcmp(d2->data(), "]")) continue;
            else if (!strcmp(d2->data(), "[")) break;
            else { float v = strtof(d2->data(), NULL); if (idx > 0) dashs[--idx] = v; }
        }
        cmd->type = TOKEN_OPERATOR_d;
        cmd->d.offset = offset;
        cmd->d.dashs[0] = dashs[0]; cmd->d.dashs[1] = dashs[1];
    } else {
        PDF_RENDERER_CALL(context->renderer, set_dash_offset, cmd->d.offset);
        PDF_RENDERER_CALL(context->renderer, set_dash_array, cmd->d.dashs, 2);
    }
}

void handle_M(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        cmd->floatVal = pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_M;
    } else {
        PDF_RENDERER_CALL(context->renderer, set_miter_limit, cmd->floatVal);
        context->state->miterLimit = cmd->floatVal;
    }
}

void handle_j(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        cmd->intVal = (int)pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_j;
    } else {
        PDF_RENDERER_CALL(context->renderer, set_line_join, cmd->intVal);
        context->state->lineJoin = cmd->intVal;
    }
}

void handle_J(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        cmd->intVal = (int)pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_J;
    } else {
        PDF_RENDERER_CALL(context->renderer, set_line_cap, cmd->intVal);
        context->state->lineCap = cmd->intVal;
    }
}

void handle_W_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { cmd->type = TOKEN_OPERATOR_W_star; }
    else { do_path(context, PDF_OPERATION_PATH_EVEN_ODD | PDF_OPERATION_PATH_CLIP); }
}
void handle_W(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { cmd->type = TOKEN_OPERATOR_W; }
    else { do_path(context, PDF_OPERATION_PATH_NON_ZERO | PDF_OPERATION_PATH_CLIP); }
}
void handle_BDC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        auto data = context->deque->pop_front();
        if (!strcmp(data->data(), ">>")) {
            context->deque->pop_front(); context->deque->pop_front(); context->deque->pop_front();
        } else { context->deque->pop_front(); }
        context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_BDC;
    }
}
void handle_BI(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        auto data = context->deque->pop_front();
        char** pptr = ((char**)(void*)data->data());
        char* buffer = *pptr;
        auto d1 = context->deque->pop_front(); int width = *(int*)(void*)d1->data();
        auto d2 = context->deque->pop_front(); int height = *(int*)(void*)d2->data();
        auto d3 = context->deque->pop_front(); int channels = *(int*)(void*)d3->data();
        auto d4 = context->deque->pop_front(); int size = *(int*)(void*)d4->data();
        cmd->type = TOKEN_OPERATOR_Do;
        cmd->Do.type = XOBJ_IMAGE;
        cmd->Do.width = width; cmd->Do.height = height;
        cmd->Do.channels = channels;
        cmd->Do.pixels = (unsigned char*)buffer; cmd->Do.pixels_size = size;
    }
}
void handle_BMC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_BMC; }
}
void handle_d0(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { pop_float(context->deque); pop_float(context->deque); cmd->type = TOKEN_OPERATOR_d0; }
}
void handle_d1(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_float(context->deque); pop_float(context->deque); pop_float(context->deque);
        pop_float(context->deque); pop_float(context->deque); pop_float(context->deque);
        cmd->type = TOKEN_OPERATOR_d1;
    }
}
void handle_DP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_DP; }
}
void handle_EMC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { cmd->type = TOKEN_OPERATOR_EMC; }
}
void handle_gs(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_gs; }
}
void handle_i(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_i; }
}
void handle_q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { cmd->type = TOKEN_OPERATOR_q; }
    else {
        PDF_RENDERER_CALL(context->renderer, save);
        if (context->state == NULL) _init_state(context);
        int depth = context->state_stack_depth + 1;
        if (depth < 32) {
            context->state_stack[depth] = *context->state;
            context->state_stack[depth].next = context->state;
            context->state = &context->state_stack[depth];
            context->state_stack_depth = depth;
        }
    }
}
void handle_Q(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { cmd->type = TOKEN_OPERATOR_Q; }
    else {
        PDF_RENDERER_CALL(context->renderer, restore);
        if (context->state_stack_depth > 0) {
            context->state = context->state->next;
            context->state_stack_depth--;
        }
        if (context->state == NULL) _init_state(context);
        PDF_RENDERER_CALL(context->renderer, set_line_width, context->state->lineWidth);
        PDF_RENDERER_CALL(context->renderer, set_line_cap, context->state->lineCap);
        PDF_RENDERER_CALL(context->renderer, set_line_join, context->state->lineJoin);
        PDF_RENDERER_CALL(context->renderer, set_miter_limit, context->state->miterLimit);
        {
            float sr, sg, sb;
            PDF_RENDERER_CALL(context->renderer, get_color, &sr, &sg, &sb);
            PDF_RENDERER_CALL(context->renderer, set_color,
                context->state->fill.color[0], context->state->fill.color[1], context->state->fill.color[2]);
        }
    }
}
void handle_MP(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_MP; }
}
void handle_ri(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_ri; }
}
void handle_sh(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) { context->deque->pop_front(); cmd->type = TOKEN_OPERATOR_sh; }
}