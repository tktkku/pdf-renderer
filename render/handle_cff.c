#include "render.h"
#include "pdf-private.h"
#include <math.h>
void handle_hstem(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    if (deque->size % 2 != 0 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        } 
    }
    context->stems += deque->size / 2;
    while (deque->size >= 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
    }
}
void handle_vstem(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    if (deque->size % 2 != 0 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        }
    }
    context->stems += deque->size / 2;
    while (deque->size >= 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data); 
    }
}
void handle_vmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size > 1 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        }
        
    }
    pdf_deque_pop_end(deque, &node);
    double dy1 = *((double*)data);
    float x = context->curX, y = context->curY;
    //plutovg_canvas_get_current_point(canvas, &x, &y);
    y += dy1;
    // if (context->open)
    // {
    //     plutovg_canvas_close_path(context->canvas);
    // }
    plutovg_canvas_move_to(canvas, x, y);
    context->open = true;
    context->curX = x;
    context->curY = y;
}
void handle_rlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = context->curX, cur_y = context->curY;
    plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    while (deque->size >= 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dx1 = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dy1 = *((double*)data);

        cur_x += dx1;
        cur_y += dy1;
        plutovg_canvas_line_to(canvas, cur_x, cur_y);
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = context->curX, cury = context->curY;
    bool horizontal = true;
    double d = 0;
    while (deque->size >= 1)
    {
        pdf_deque_pop_end(deque, &node);
        d = *((double*)data);
        if (horizontal)
        {
            curx += d;
        }
        else
        {
            cury += d;
        }
        plutovg_canvas_line_to(canvas, curx, cury);
        horizontal = !horizontal;
    }
    
    context->curX = curx;
    context->curY = cury;
}
void handle_vlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = context->curX, cury = context->curY;
    bool vertical = true;
    double d = 0;
    while (deque->size >= 1)
    {
        pdf_deque_pop_end(deque, &node);
        d = *((double*)data);
        if (vertical)
        {
            cury += d;
        }
        else
        {
            curx += d;
        }
        plutovg_canvas_line_to(canvas, curx, cury);
        vertical = !vertical;
    }

    context->curX = curx;
    context->curY = cury;
}
void handle_rrcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = context->curX, cury = context->curY;
    //plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size >= 6)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxc = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyc = *((double*)data);

        double c1x = curx + dxa; double c1y = cury + dya;
        double c2x = c1x + dxb; double c2y = c1y + dyb;
        double c3x = c2x + dxc; double c3y = c2y + dyc;

        plutovg_canvas_cubic_to(canvas, c1x, c1y, c2x, c2y, c3x, c3y);
        curx = c3x;
        cury = c3y;
    }
    context->curX = curx;
    context->curY = cury;
}
void handle_callsubr(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    pdf_deque_pop_front(deque, &node);
    double g = *((double*)data);
}
void handle_hstemhm(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 != 0 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        }
    }
    context->stems += deque->size / 2;
    context->stemshm += deque->size / 2;
    while (deque->size >= 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
    }
}
void handle_hintmask(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    int count = 1 + floor((context->stemshm - 1) / 8);
    context->cur += count;
}
void handle_cntrmask(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    int count = 1 + floor((context->stems - 1) / 8);
    context->cur += count;
}
void handle_rmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 != 0 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        }
        
    }
    pdf_deque_pop_end(deque, &node);
    double dx1 = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dy1 = *((double*)data);
    float curx = context->curX, cury = context->curY;
    curx += dx1;
    cury += dy1;
    // if (context->open)
    // {
    //     plutovg_canvas_close_path(context->canvas);
    // }
    plutovg_canvas_move_to(canvas, curx, cury);
    context->open = true;
    context->curX = curx;
    context->curY = cury;
}
void handle_hmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size > 1 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        if (!context->havewidth)
        {
            double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
            context->width = width;
            context->havewidth = true;
        }
        
    }
    float x = context->curX, y = context->curY;
    //plutovg_canvas_get_current_point(canvas, &x, &y);
    pdf_deque_pop_end(deque, &node);
    double dx1 = *((double*)data);
    x += dx1;
    // if (context->open)
    // {
    //     plutovg_canvas_close_path(context->canvas);
    // }
    plutovg_canvas_move_to(canvas, x, y);
    context->open = true;
    context->curX = x;
    context->curY = y;
}
void handle_vstemhm(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 != 0 && !context->havewidth)
    {
        pdf_deque_pop_end(deque, &node);
        double width = fabs(*((double*)data)) * context->fontSize / 1000.0;
        context->width = width;
        context->havewidth = true;
    }
    context->stems += deque->size / 2;
    context->stemshm += deque->size / 2;
    while (deque->size >= 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data);
    }
}
void handle_rcurveline(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = context->curX, cury = context->curY;
    // plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size > 2)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxc = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyc = *((double*)data);

        double c1x = curx + dxa; double c1y = cury + dya;
        double c2x = c1x + dxb; double c2y = c1y + dyb;
        double c3x = c2x + dxc; double c3y = c2y + dyc;

        plutovg_canvas_cubic_to(canvas, c1x, c1y, c2x, c2y, c3x, c3y);
        curx = c3x;
        cury = c3y;
    }
    pdf_deque_pop_end(deque, &node);
    double dxd = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dyd = *((double*)data);
    curx += dxd;
    cury += dyd;
    plutovg_canvas_line_to(canvas, curx, cury);
    context->curX = curx;
    context->curY = cury;
}
void handle_rlinecurve(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = context->curX, cury = context->curY;
    // plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size > 6)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        curx += dxa;
        cury += dya;
        plutovg_canvas_line_to(canvas, curx, cury);
    }

    pdf_deque_pop_end(deque, &node);
    double dxb = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dyb = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dxc = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dyc = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dxd = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dyd = *((double*)data);

    double c1x = curx + dxb; double c1y = cury + dyb;
    double c2x = c1x + dxc; double c2y = c1y + dyc;
    double c3x = c2x + dxd; double c3y = c2y + dyd;

    plutovg_canvas_cubic_to(canvas, c1x, c1y, c2x, c2y, c3x, c3y);
    context->curX = c3x;
    context->curY = c3y;
}
void handle_vvcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = context->curX, cur_y = context->curY;
    // plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    double dx1;
    if (deque->size % 2)
    {
        pdf_deque_pop_end(deque, &node);
        dx1 = *((double*)data);
        cur_x += dx1;
    }
    double dya, dxb, dyb, dyc;
    while (deque->size >= 4)
    {
        pdf_deque_pop_end(deque, &node);
        dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dxb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dyb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dyc = *((double*)data);

        double cp1_x = cur_x;
        double cp1_y = cur_y + dya;
        double cp2_x = cp1_x + dxb;
        double cp2_y = cp1_y + dyb;
        double end_x = cp2_x;
        double end_y = cp2_y + dyc;

        plutovg_canvas_cubic_to(canvas, cp1_x, cp1_y, cp2_x, cp2_y, end_x, end_y);
        cur_x = end_x;
        cur_y = end_y;
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hhcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = context->curX, cur_y = context->curY;
    //plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    double dy1;
    if (deque->size % 2)
    {
        pdf_deque_pop_end(deque, &node);
        dy1 = *((double*)data);
        cur_y += dy1;
    }
    double dxa, dxb, dyb, dxc;
    while (deque->size >= 4)
    {
        pdf_deque_pop_end(deque, &node);
        dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dxb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dyb = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        dxc = *((double*)data);

        double cp1_x = cur_x + dxa;
        double cp1_y = cur_y;
        double cp2_x = cp1_x + dxb;
        double cp2_y = cp1_y + dyb;
        double end_x = cp2_x + dxc;
        double end_y = cp2_y;

        plutovg_canvas_cubic_to(canvas, cp1_x, cp1_y, cp2_x, cp2_y, end_x, end_y);
        cur_x = end_x;
        cur_y = end_y;
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_callgsubr(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    pdf_array_t* global_subr_index = context->global_subr;
    uint16_t global_bias = context->global_bias;

    pdf_deque_pop_front(deque, &node);
    double g = *((double*)data);
    uint32_t off = g + global_bias;
    pdf_cff_char_render_t ctx;
    int savelen = context->len;
    unsigned char* savebuf = context->buf;
    unsigned char* savecur = context->cur;
    context->buf = global_subr_index->values[off]->val.string;
    context->cur = context->buf;
    context->len = global_subr_index->values[off]->value_len;
    _cff_do_render_char(context, deque);
    context->buf = savebuf;
    context->cur = savecur;
    context->len = savelen;
}
void handle_vhcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1)

    float cur_x = context->curX, cur_y = context->curY;
    bool vertical = true;
    double x1, y1, x2, y2, x3, y3;
    double d = 0;
    while (deque->size >= 4)
    {
        double params[4];
        for (int i = 0; i < 4; i++)
        {
            pdf_deque_pop_end(deque, &node);
            params[i] = *((double*)data);
        }
        if (deque->size != 1)
        {
            d = 0;
        }
        else
        {
            pdf_deque_pop_end(deque, &node);
            d = *((double*)data);
        }
        if (vertical)
        {
            x1 = cur_x; y1 = cur_y + params[0];
            x2 = x1 + params[1]; y2 = y1 + params[2];
            x3 = x2 + params[3]; y3 = y2 + d;
        }
        else
        {
            x1 = cur_x + params[0]; y1 = cur_y;
            x2 = x1 + params[1]; y2 = y1 + params[2];
            x3 = x2 + d; y3 = y2 + params[3];
        }
        vertical = !vertical;
        plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        cur_x = x3;
        cur_y = y3;
    }

    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hvcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1) 4,5,12,13...
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1) 8,9,16,17... 
    float cur_x = context->curX, cur_y = context->curY;
    bool horizontal = true;
    double x1, y1, x2, y2, x3, y3;
    double d = 0;
    while (deque->size >= 4)
    {
        double params[4];
        for (int i = 0; i < 4; i++)
        {
            pdf_deque_pop_end(deque, &node);
            params[i] = *((double*)data);
        }
        if (deque->size != 1)
        {
            d = 0; 
        }
        else
        { 
            pdf_deque_pop_end(deque, &node);
            d = *((double*)data);
        }
        if (horizontal)
        {
            x1 = cur_x + params[0]; y1 = cur_y;
            x2 = x1 + params[1]; y2 = y1 + params[2];
            x3 = x2 + d; y3 = y2 + params[3];
        }
        else
        {
            x1 = cur_x; y1 = cur_y + params[0];
            x2 = x1 + params[1]; y2 = y1 + params[2];
            x3 = x2 + params[3]; y3 = y2 + d;
        }
        horizontal = !horizontal;
        plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        cur_x = x3;
        cur_y = y3;
    }

    context->curX = cur_x;
    context->curY = cur_y;
}

void handle_and(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("and not implemented\n");}
void handle_or(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("or not implemented\n");}
void handle_not(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{ printf("not not implemented\n");}
void handle_abs(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("abs not implemented\n");}
void handle_add(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("add not implemented\n");}
void handle_sub(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("sub not implemented\n");}
void handle_div(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("div not implemented\n");}
void handle_neg(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("neg not implemented\n");}
void handle_eq(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("eq not implemented\n");}
void handle_drop(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("drop not implemented\n");}
void handle_put(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("put not implemented\n");}
void handle_get(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("get not implemented\n");}
void handle_ifelse(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("ifelse not implemented\n");}
void handle_random(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("random not implemented\n");}
void handle_mul(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("mul not implemented\n");}
void handle_sqrt(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("sqrt not implemented\n");}
void handle_dup(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("dup not implemented\n");}
void handle_exch(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("exch not implemented\n");}
void handle_index(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("index not implemented\n");}
void handle_roll(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("roll not implemented\n");}
void handle_hflex(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("hflex not implemented\n");}
void handle_flex(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("flex not implemented\n");}
void handle_hflex1(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("hflex1 not implemented\n");}
void handle_flex1(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{printf("flex1 not implemented\n");}
