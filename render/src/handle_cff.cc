#include "pdf-render.h"
#include "pdf-render-private.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>
void handle_hstem(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() % 2 != 0 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        } 
    }
    context->stems += deque->size() / 2;
    while (deque->size() >= 2)
    {
        auto data = deque->pop_back();
        double dya = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dyb = *((double*)data2->data());
    }
}
void handle_vstem(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() % 2 != 0 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        }
    }
    context->stems += deque->size() / 2;
    while (deque->size() >= 2)
    {
        auto data = deque->pop_back();
        double dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dxb = *((double*)data2->data()); 
    }
}
void handle_vmoveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() > 1 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        }
        
    }
    auto data = deque->pop_back();
    double dy1 = *((double*)data->data());
    float x = context->curX, y = context->curY;
    y += dy1;
    PDF_RENDERER_CALL(context->renderer, move_to, x, y);
    context->open = true;
    context->curX = x;
    context->curY = y;
}
void handle_rlineto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float cur_x = context->curX, cur_y = context->curY;
    PDF_RENDERER_CALL(context->renderer, get_current_point, &cur_x, &cur_y);
    while (deque->size() >= 2)
    {
        auto data = deque->pop_back();
        double dx1 = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dy1 = *((double*)data2->data());

        cur_x += dx1;
        cur_y += dy1;
        PDF_RENDERER_CALL(context->renderer, line_to, cur_x, cur_y);
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hlineto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float curx = context->curX, cury = context->curY;
    bool horizontal = true;
    double d = 0;
    while (deque->size() >= 1)
    {
        auto data = deque->pop_back();
        d = *((double*)data->data());
        if (horizontal)
        {
            curx += d;
        }
        else
        {
            cury += d;
        }
        PDF_RENDERER_CALL(context->renderer, line_to, curx, cury);
        horizontal = !horizontal;
    }
    
    context->curX = curx;
    context->curY = cury;
}
void handle_vlineto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float curx = context->curX, cury = context->curY;
    bool vertical = true;
    double d = 0;
    while (deque->size() >= 1)
    {
        auto data = deque->pop_back();
        d = *((double*)data->data());
        if (vertical)
        {
            cury += d;
        }
        else
        {
            curx += d;
        }
        PDF_RENDERER_CALL(context->renderer, line_to, curx, cury);
        vertical = !vertical;
    }

    context->curX = curx;
    context->curY = cury;
}
void handle_rrcurveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float curx = context->curX, cury = context->curY;
    while (deque->size() >= 6)
    {
        auto data = deque->pop_back();
        double dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dya = *((double*)data2->data());
        auto data3 = deque->pop_back();
        double dxb = *((double*)data3->data());
        auto data4 = deque->pop_back();
        double dyb = *((double*)data4->data());
        auto data5 = deque->pop_back();
        double dxc = *((double*)data5->data());
        auto data6 = deque->pop_back();
        double dyc = *((double*)data6->data());

        double c1x = curx + dxa; double c1y = cury + dya;
        double c2x = c1x + dxb; double c2y = c1y + dyb;
        double c3x = c2x + dxc; double c3y = c2y + dyc;

        PDF_RENDERER_CALL(context->renderer, cubic_to, c1x, c1y, c2x, c2y, c3x, c3y);
        curx = c3x;
        cury = c3y;
    }
    context->curX = curx;
    context->curY = cury;
}
void handle_callsubr(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    // ponytail: implement local subroutine call (was stub — CFF Type1 subrs broken)
    pdf_array* local_subr_index = context->charstrings;
    if (local_subr_index == NULL) return;

    // compute local bias (same formula as global, but based on local index size)
    uint16_t local_bias = 0;
    size_t count = local_subr_index->size();
    if (count < 1240) local_bias = 0;
    else if (count < 33900) local_bias = 1131;
    else local_bias = 32768;

    auto data = deque->pop_front();
    double g = *((double*)data->data());
    uint32_t off = (uint32_t)g + local_bias;
    if (off >= count) return;

    int savelen = context->len;
    unsigned char* savebuf = context->buf;
    unsigned char* savecur = context->cur;
    context->buf = (unsigned char*)local_subr_index->get(off)->val.string;
    context->cur = context->buf;
    context->len = local_subr_index->get(off)->value_len;
    _cff_do_render_char(context, deque);
    context->buf = savebuf;
    context->cur = savecur;
    context->len = savelen;
}
void handle_hstemhm(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() % 2 != 0 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        }
    }
    context->stems += deque->size() / 2;
    context->stemshm += deque->size() / 2;
    while (deque->size() >= 2)
    {
        auto data = deque->pop_back();
        double dya = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dyb = *((double*)data2->data());
    }
}
void handle_hintmask(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    int count = 1 + floor((context->stemshm - 1) / 8);
    context->cur += count;
}
void handle_cntrmask(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    int count = 1 + floor((context->stems - 1) / 8);
    context->cur += count;
}
void handle_rmoveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() % 2 != 0 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        }
        
    }
    auto data = deque->pop_back();
    double dx1 = *((double*)data->data());
    auto data2 = deque->pop_back();
    double dy1 = *((double*)data2->data());
    float curx = context->curX, cury = context->curY;
    curx += dx1;
    cury += dy1;
    PDF_RENDERER_CALL(context->renderer, move_to, curx, cury);
    context->open = true;
    context->curX = curx;
    context->curY = cury;
}
void handle_hmoveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() > 1 && !context->havewidth)
    {
        auto data = deque->pop_back();
        if (!context->havewidth)
        {
            double width = *((double*)data->data());
            context->width = width;
            context->havewidth = true;
        }
        
    }
    float x = context->curX, y = context->curY;
    auto data = deque->pop_back();
    double dx1 = *((double*)data->data());
    x += dx1;
    PDF_RENDERER_CALL(context->renderer, move_to, x, y);
    context->open = true;
    context->curX = x;
    context->curY = y;
}
void handle_vstemhm(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    if (deque->size() % 2 != 0 && !context->havewidth)
    {
        auto data = deque->pop_back();
        double width = *((double*)data->data());
        context->width = width;
        context->havewidth = true;
    }
    context->stems += deque->size() / 2;
    context->stemshm += deque->size() / 2;
    while (deque->size() >= 2)
    {
        auto data = deque->pop_back();
        double dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dxb = *((double*)data2->data());
    }
}
void handle_rcurveline(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float curx = context->curX, cury = context->curY;
    while (deque->size() > 2)
    {
        auto data = deque->pop_back();
        double dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dya = *((double*)data2->data());
        auto data3 = deque->pop_back();
        double dxb = *((double*)data3->data());
        auto data4 = deque->pop_back();
        double dyb = *((double*)data4->data());
        auto data5 = deque->pop_back();
        double dxc = *((double*)data5->data());
        auto data6 = deque->pop_back();
        double dyc = *((double*)data6->data());

        double c1x = curx + dxa; double c1y = cury + dya;
        double c2x = c1x + dxb; double c2y = c1y + dyb;
        double c3x = c2x + dxc; double c3y = c2y + dyc;

        PDF_RENDERER_CALL(context->renderer, cubic_to, c1x, c1y, c2x, c2y, c3x, c3y);
        curx = c3x;
        cury = c3y;
    }
    auto data = deque->pop_back();
    double dxd = *((double*)data->data());
    auto data2 = deque->pop_back();
    double dyd = *((double*)data2->data());
    curx += dxd;
    cury += dyd;
    PDF_RENDERER_CALL(context->renderer, line_to, curx, cury);
    context->curX = curx;
    context->curY = cury;
}
void handle_rlinecurve(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float curx = context->curX, cury = context->curY;
    while (deque->size() > 6)
    {
        auto data = deque->pop_back();
        double dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        double dya = *((double*)data2->data());
        curx += dxa;
        cury += dya;
        PDF_RENDERER_CALL(context->renderer, line_to, curx, cury);
    }

    auto data = deque->pop_back();
    double dxb = *((double*)data->data());
    auto data2 = deque->pop_back();
    double dyb = *((double*)data2->data());
    auto data3 = deque->pop_back();
    double dxc = *((double*)data3->data());
    auto data4 = deque->pop_back();
    double dyc = *((double*)data4->data());
    auto data5 = deque->pop_back();
    double dxd = *((double*)data5->data());
    auto data6 = deque->pop_back();
    double dyd = *((double*)data6->data());

    double c1x = curx + dxb; double c1y = cury + dyb;
    double c2x = c1x + dxc; double c2y = c1y + dyc;
    double c3x = c2x + dxd; double c3y = c2y + dyd;

    PDF_RENDERER_CALL(context->renderer, cubic_to, c1x, c1y, c2x, c2y, c3x, c3y);
    context->curX = c3x;
    context->curY = c3y;
}
void handle_vvcurveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float cur_x = context->curX, cur_y = context->curY;
    double dx1;
    if (deque->size() % 2)
    {
        auto data = deque->pop_back();
        dx1 = *((double*)data->data());
        cur_x += dx1;
    }
    double dya, dxb, dyb, dyc;
    while (deque->size() >= 4)
    {
        auto data = deque->pop_back();
        dya = *((double*)data->data());
        auto data2 = deque->pop_back();
        dxb = *((double*)data2->data());
        auto data3 = deque->pop_back();
        dyb = *((double*)data3->data());
        auto data4 = deque->pop_back();
        dyc = *((double*)data4->data());

        double cp1_x = cur_x;
        double cp1_y = cur_y + dya;
        double cp2_x = cp1_x + dxb;
        double cp2_y = cp1_y + dyb;
        double end_x = cp2_x;
        double end_y = cp2_y + dyc;

        PDF_RENDERER_CALL(context->renderer, cubic_to, cp1_x, cp1_y, cp2_x, cp2_y, end_x, end_y);
        cur_x = end_x;
        cur_y = end_y;
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hhcurveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    float cur_x = context->curX, cur_y = context->curY;
    double dy1;
    if (deque->size() % 2)
    {
        auto data = deque->pop_back();
        dy1 = *((double*)data->data());
        cur_y += dy1;
    }
    double dxa, dxb, dyb, dxc;
    while (deque->size() >= 4)
    {
        auto data = deque->pop_back();
        dxa = *((double*)data->data());
        auto data2 = deque->pop_back();
        dxb = *((double*)data2->data());
        auto data3 = deque->pop_back();
        dyb = *((double*)data3->data());
        auto data4 = deque->pop_back();
        dxc = *((double*)data4->data());

        double cp1_x = cur_x + dxa;
        double cp1_y = cur_y;
        double cp2_x = cp1_x + dxb;
        double cp2_y = cp1_y + dyb;
        double end_x = cp2_x + dxc;
        double end_y = cp2_y;

        PDF_RENDERER_CALL(context->renderer, cubic_to, cp1_x, cp1_y, cp2_x, cp2_y, end_x, end_y);
        cur_x = end_x;
        cur_y = end_y;
    }
    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_callgsubr(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    pdf_array* global_subr_index = context->global_subr;
    uint16_t global_bias = context->global_bias;

    auto data = deque->pop_front();
    double g = *((double*)data->data());
    uint32_t off = g + global_bias;
    pdf_cff_char_render_t ctx;
    int savelen = context->len;
    unsigned char* savebuf = context->buf;
    unsigned char* savecur = context->cur;
    context->buf = (unsigned char*)global_subr_index->get(off)->val.string;
    context->cur = context->buf;
    context->len = global_subr_index->get(off)->value_len;
    _cff_do_render_char(context, deque);
    context->buf = savebuf;
    context->cur = savecur;
    context->len = savelen;
}
void handle_vhcurveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1)

    float cur_x = context->curX, cur_y = context->curY;
    bool vertical = true;
    double x1, y1, x2, y2, x3, y3;
    double d = 0;
    while (deque->size() >= 4)
    {
        double params[4];
        for (int i = 0; i < 4; i++)
        {
            auto data = deque->pop_back();
            params[i] = *((double*)data->data());
        }
        if (deque->size() != 1)
        {
            d = 0;
        }
        else
        {
            auto data = deque->pop_back();
            d = *((double*)data->data());
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
        PDF_RENDERER_CALL(context->renderer, cubic_to, x1, y1, x2, y2, x3, y3);
        cur_x = x3;
        cur_y = y3;
    }

    context->curX = cur_x;
    context->curY = cur_y;
}
void handle_hvcurveto(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1) 4,5,12,13...
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1) 8,9,16,17... 
    float cur_x = context->curX, cur_y = context->curY;
    bool horizontal = true;
    double x1, y1, x2, y2, x3, y3;
    double d = 0;
    while (deque->size() >= 4)
    {
        double params[4];
        for (int i = 0; i < 4; i++)
        {
            auto data = deque->pop_back();
            params[i] = *((double*)data->data());
        }
        if (deque->size() != 1)
        {
            d = 0; 
        }
        else
        { 
            auto data = deque->pop_back();
            d = *((double*)data->data());
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
        PDF_RENDERER_CALL(context->renderer, cubic_to, x1, y1, x2, y2, x3, y3);
        cur_x = x3;
        cur_y = y3;
    }

    context->curX = cur_x;
    context->curY = cur_y;
}

void handle_and(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    int num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    int num2 = *((double*)data2->data());
    double v = 0;
    if (num1 != 0 && num2 != 0)
    {
        v = 1;
    }
    else
    {
        v = 0;
    }
    deque->push_front(&v, sizeof(double));
}
void handle_or(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    int num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    int num2 = *((double*)data2->data());
    double v = 0;
    if (num1 == 0 && num2 == 0)
    {
        v = 0;
    }
    else
    {
        v = 1;
    }
    deque->push_front(&v, sizeof(double));
}
void handle_not(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    int num1 = *((double*)data->data());
    double v = 0;
    if (num1 != 0)
    {
        v = 0;
    }
    else
    {
        v = 1;
    }
    deque->push_front(&v, sizeof(double));
}
void handle_abs(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double v = *((double*)data->data());
    v = fabs(v);
    deque->push_front(&v, sizeof(double));
}
void handle_add(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double num2 = *((double*)data2->data());
    double sum = num1 + num2;
    deque->push_front(&sum, sizeof(double));
}
void handle_sub(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double num2 = *((double*)data2->data());
    double sub = num1 - num2;
    deque->push_front(&sub, sizeof(double));
}
void handle_div(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double num2 = *((double*)data2->data());
    double div = num1 / num2;
    deque->push_front(&div, sizeof(double));
}
void handle_neg(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double num1 = *((double*)data->data());
    num1 = -num1;
    deque->push_front(&num1, sizeof(double));
}
void handle_eq(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double num1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double num2 = *((double*)data2->data());
    double v = 0;
    if (num1 == num2)
    {
        v = 1;
    }
    else
    {
        v = 0;
    }
    deque->push_front(&v, sizeof(double));
}
void handle_drop(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
}
void handle_put(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double val = *((double*)data->data());
    auto data2 = deque->pop_front();
    int i = *((double*)data2->data());
    context->transient[i] = val;
}
void handle_get(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    int i = *((double*)data->data());
    double val = context->transient[i];
    deque->push_front(&val, sizeof(double));
}
void handle_ifelse(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double s1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double s2 = *((double*)data2->data());
    auto data3 = deque->pop_front();
    double v1 = *((double*)data3->data());
    auto data4 = deque->pop_front();
    double v2 = *((double*)data4->data());
    if (v1 <= v2)
    {
        deque->push_front(&s1, sizeof(double));
    }
    else
    {
        deque->push_front(&s2, sizeof(double));
    }
    
}
void handle_random(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    srand((unsigned int)time(NULL));
    const int max = 1000;
    int limit = RAND_MAX - (RAND_MAX % max);
    int num;

    do 
    {
        num = rand();
    } while (num >= limit); // num >= 1000 * n

    num = (num % max) + 1;// from 1 to 1000
    double random_num = num / 1000.0;
    deque->push_front(&random_num, sizeof(double));
}
void handle_mul(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double s1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double s2 = *((double*)data2->data());
    double s3 = s1 * s2;
    deque->push_front(&s3, sizeof(double));
}
void handle_sqrt(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double s1 = *((double*)data->data());
    double s2 = sqrt(s1);
    deque->push_front(&s2, sizeof(double));
}
void handle_dup(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double s1 = *((double*)data->data());
    deque->push_front(&s1, sizeof(double));
    deque->push_front(&s1, sizeof(double));
}
void handle_exch(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    double s1 = *((double*)data->data());
    auto data2 = deque->pop_front();
    double s2 = *((double*)data2->data());
    deque->push_front(&s1, sizeof(double));
    deque->push_front(&s2, sizeof(double));
}
void handle_index(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    auto data = deque->pop_front();
    int index = *((double*)data->data());
    auto data2 = deque->get(index);
    double v = *((double*)data2.data());
    deque->push_front(&v, sizeof(double));
}
void handle_roll(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    // ponytail: implement roll operator — rotate top N elements by J
    auto jdata = deque->pop_front();
    auto ndata = deque->pop_front();
    if (!jdata || !ndata) return;
    int J = (int)(*((double*)jdata->data()));
    int N = (int)(*((double*)ndata->data()));
    if (N <= 0) return;
    // modulo N so we only rotate within the N elements
    J = J % N;
    if (J == 0) return;
    // pop N elements into temp array
    double* tmp = (double*)alloca(N * sizeof(double));
    for (int i = N - 1; i >= 0; i--)
    {
        auto d = deque->pop_front();
        tmp[i] = *((double*)d->data());
    }
    // rotate and push back: after rotation by J, element at (i+J)%N moves to position i
    for (int i = N - 1; i >= 0; i--)
    {
        double v = tmp[(i - J + N) % N];
        deque->push_front(&v, sizeof(double));
    }
}
// ponytail: shared flex curve renderer — eliminates 4x duplicated depth-check + line/curve code
static void _render_flex_curve(pdf_cff_char_render_t* context,
    double c1x, double c1y, double c2x, double c2y, double c3x, double c3y,
    double c4x, double c4y, double c5x, double c5y, double c6x, double c6y, int fd)
{
    double depth = fabs((c6y - c1y) * c3x - (c6x - c1x) * c3y + c6x * c1y - c6y * c1x)
        / sqrt((c6x - c1x) * (c6x - c1x) + (c6y - c1y) * (c6y - c1y));
    if (depth < (fd / 100.0))
        PDF_RENDERER_CALL(context->renderer, line_to, c6x, c6y);
    else {
        PDF_RENDERER_CALL(context->renderer, cubic_to, c1x, c1y, c2x, c2y, c3x, c3y);
        PDF_RENDERER_CALL(context->renderer, cubic_to, c4x, c4y, c5x, c5y, c6x, c6y);
    }
}
void handle_hflex(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    double curx = context->curX;
    double cury = context->curY;
    if (deque->size() == 7)
    {
        double dx1 = *((double*)deque->pop_front()->data());
        double dx2 = *((double*)deque->pop_front()->data());
        double dy2 = *((double*)deque->pop_front()->data());
        double dx3 = *((double*)deque->pop_front()->data());
        double dx4 = *((double*)deque->pop_front()->data());
        double dx5 = *((double*)deque->pop_front()->data());
        double dx6 = *((double*)deque->pop_front()->data());
        // ponytail: compact flex coordinate calculation
        double c1x = curx + dx1, c1y = cury;
        double c2x = c1x + dx2, c2y = c1y + dy2;
        double c3x = c2x + dx3, c3y = c2y + dy2;
        double c4x = c3x + dx4, c4y = c3y + dy2;
        double c5x = c4x + dx5, c5y = c4y;
        double c6x = c5x + dx6, c6y = c5y;
        context->curX = c6x; context->curY = c6y;
        _render_flex_curve(context, c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y, c5x, c5y, c6x, c6y, 50);
    }
    deque->clear();
}
void handle_flex(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    double curx = context->curX, cury = context->curY;
    if (deque->size() == 13)
    {
        double dx1 = *((double*)deque->pop_front()->data());
        double dy1 = *((double*)deque->pop_front()->data());
        double dx2 = *((double*)deque->pop_front()->data());
        double dy2 = *((double*)deque->pop_front()->data());
        double dx3 = *((double*)deque->pop_front()->data());
        double dy3 = *((double*)deque->pop_front()->data());
        double dx4 = *((double*)deque->pop_front()->data());
        double dy4 = *((double*)deque->pop_front()->data());
        double dx5 = *((double*)deque->pop_front()->data());
        double dy5 = *((double*)deque->pop_front()->data());
        double dx6 = *((double*)deque->pop_front()->data());
        double dy6 = *((double*)deque->pop_front()->data());
        double fd = *((double*)deque->pop_front()->data());
        double c1x = curx + dx1, c1y = cury + dy1;
        double c2x = c1x + dx2, c2y = c1y + dy2;
        double c3x = c2x + dx3, c3y = c2y + dy3;
        double c4x = c3x + dx4, c4y = c3y + dy4;
        double c5x = c4x + dx5, c5y = c4y + dy5;
        double c6x = c5x + dx6, c6y = c5y + dy6;
        context->curX = c6x; context->curY = c6y;
        _render_flex_curve(context, c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y, c5x, c5y, c6x, c6y, (int)fd);
    }
    deque->clear();
}
void handle_hflex1(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    double curx = context->curX, cury = context->curY;
    if (deque->size() == 9)
    {
        double dx1 = *((double*)deque->pop_front()->data());
        double dy1 = *((double*)deque->pop_front()->data());
        double dx2 = *((double*)deque->pop_front()->data());
        double dy2 = *((double*)deque->pop_front()->data());
        double dx3 = *((double*)deque->pop_front()->data());
        double dx4 = *((double*)deque->pop_front()->data());
        double dx5 = *((double*)deque->pop_front()->data());
        double dy5 = *((double*)deque->pop_front()->data());
        double dx6 = *((double*)deque->pop_front()->data());
        double c1x = curx + dx1, c1y = cury + dy1;
        double c2x = c1x + dx2, c2y = c1y + dy2;
        double c3x = c2x + dx3, c3y = c2y + dy2;
        double c4x = c3x + dx4, c4y = c3y + dy2;
        double c5x = c4x + dx5, c5y = c4y + dy5;
        double c6x = c5x + dx6, c6y = cury;
        context->curX = c6x; context->curY = c6y;
        _render_flex_curve(context, c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y, c5x, c5y, c6x, c6y, 50);
    }
    deque->clear();
}
void handle_flex1(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    double curx = context->curX, cury = context->curY;
    if (deque->size() == 11)
    {
        double dx1 = *((double*)deque->pop_front()->data());
        double dy1 = *((double*)deque->pop_front()->data());
        double dx2 = *((double*)deque->pop_front()->data());
        double dy2 = *((double*)deque->pop_front()->data());
        double dx3 = *((double*)deque->pop_front()->data());
        double dy3 = *((double*)deque->pop_front()->data());
        double dx4 = *((double*)deque->pop_front()->data());
        double dy4 = *((double*)deque->pop_front()->data());
        double dx5 = *((double*)deque->pop_front()->data());
        double dy5 = *((double*)deque->pop_front()->data());
        double d6 = *((double*)deque->pop_front()->data());
        double c1x = curx + dx1, c1y = cury + dy1;
        double c2x = c1x + dx2, c2y = c1y + dy2;
        double c3x = c2x + dx3, c3y = c2y + dy3;
        double c4x = c3x + dx4, c4y = c3y + dy4;
        double c5x = c4x + dx5, c5y = c4y + dy5;
        double c6x = (fabs(c5x) > fabs(c5y)) ? d6 : curx;
        double c6y = (fabs(c5x) > fabs(c5y)) ? cury : d6;
        context->curX = c6x; context->curY = c6y;
        _render_flex_curve(context, c1x, c1y, c2x, c2y, c3x, c3y, c4x, c4y, c5x, c5y, c6x, c6y, 50);
    }
    deque->clear();
}
