#include "render.h"
#include "pdf-private.h"
void handle_hstem(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    printf("hstem not implemented\n");
    if (deque->size % 2 != 0)
    {
        pdf_deque_pop_end(deque, &node);
    }

    pdf_deque_pop_end(deque, &node);
    double y = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dy = *((double*)data);
    while (deque->size > 0 && deque->size % 2 == 0)
    {
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
    }
    pdf_deque_empty(deque);
}
void handle_vstem(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    printf("vstem not implemented\n");
    if (deque->size % 2 != 0)
    {
        pdf_deque_pop_end(deque, &node);
    }
    pdf_deque_pop_end(deque, &node);
    double x = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dx = *((double*)data);
    while (deque->size > 0 && deque->size % 2 == 0)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data);
    }
    pdf_deque_empty(deque);
}
void handle_vmoveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size > 1)
    {
        pdf_deque_pop_end(deque, &node);
    }
    pdf_deque_pop_end(deque, &node);
    double dy1 = *((double*)data);
    float x = 0, y = 0;
    //plutovg_canvas_get_current_point(canvas, &x, &y);
    plutovg_canvas_move_to(canvas, x, y + dy1);
    pdf_deque_empty(deque);
}
void handle_rlineto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = 0, cur_y = 0;
    //plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    while (deque->size > 0)
    {
        pdf_deque_pop_end(deque, &node);
        double dx1 = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dy1 = *((double*)data);

        cur_x += dx1;
        cur_y += dy1;
        plutovg_canvas_line_to(canvas, cur_x, cur_y);
    }
    pdf_deque_empty(deque);
}
void handle_hlineto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 == 0)
    {
        float curx = 0, cury = 0;
        //plutovg_canvas_get_current_point(canvas, &curx, &curx);
        while (deque->size > 0)
        {
            pdf_deque_pop_end(deque, &node);
            double dxa = *((double*)data);
            curx += dxa;
            plutovg_canvas_line_to(canvas, curx, cury);

            pdf_deque_pop_end(deque, &node);
            double dyb = *((double*)data);
            cury += dyb;
            plutovg_canvas_line_to(canvas, curx, cury);
        }
    }
    else
    {
        float curx = 0, cury = 0;
        //plutovg_canvas_get_current_point(canvas, &curx, &cury);
        pdf_deque_pop_end(deque, &node);
        double dx1 = *((double*)data);
        curx += dx1;
        plutovg_canvas_line_to(canvas, curx, cury);
        while (deque->size > 0)
        {
            pdf_deque_pop_end(deque, &node);
            double dya = *((double*)data);
            cury += dya;
            plutovg_canvas_line_to(canvas, curx, cury);

            pdf_deque_pop_end(deque, &node);
            double dxb = *((double*)data);
            curx += dxb;
            plutovg_canvas_line_to(canvas, curx, cury);
        }
    }
    pdf_deque_empty(deque);
}
void handle_vlineto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 == 0)
    {
        float curx, cury;
        while (deque->size > 0)
        {
            pdf_deque_pop_end(deque, &node);
            double dya = *((double*)data);
            cury += dya;
            plutovg_canvas_line_to(canvas, curx, dya);

            pdf_deque_pop_end(deque, &node);
            double dxb = *((double*)data);
            curx += dxb;
            plutovg_canvas_line_to(canvas, curx, cury);
        }
    }
    else
    {
        float curx, cury;
        pdf_deque_pop_end(deque, &node);
        double dy1 = *((double*)data);
        cury += dy1;
        plutovg_canvas_line_to(canvas, curx, cury);
        while (deque->size > 0)
        {
            pdf_deque_pop_end(deque, &node);
            double dxa = *((double*)data);
            curx += dxa;
            plutovg_canvas_line_to(canvas, curx, cury);

            pdf_deque_pop_end(deque, &node);
            double dyb = *((double*)data);
            cury += dyb;
            plutovg_canvas_line_to(canvas, curx, cury);
        }
    }
    pdf_deque_empty(deque);
}
void handle_rrcurveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = 0, cury = 0;
    //plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size > 0 && deque->size % 6 == 0)
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
    pdf_deque_empty(deque);
}
void handle_callsubr(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    printf("callsubr not implemented\n");
    pdf_deque_pop_front(deque, &node);
    double g = *((double*)data);
}
void handle_hstemhm(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    printf("hstemhm not implemented\n");
    if (deque->size % 2 != 0)
    {
        pdf_deque_pop_end(deque, &node);
    }
    pdf_deque_pop_end(deque, &node);
    double y = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dy = *((double*)data);
    while (deque->size > 0 && deque->size % 2 == 0)
    {
        pdf_deque_pop_end(deque, &node);
        double dya = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dyb = *((double*)data);
    }
    pdf_deque_empty(deque);
}
void handle_hintmask(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    printf("hintmask not implemented\n");
    pdf_deque_empty(deque);
}
void handle_cntrmask(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    printf("cntrmask not implemented\n");
    pdf_deque_empty(deque);
}
void handle_rmoveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size % 2 != 0)
    {
        pdf_deque_pop_end(deque, &node);
    }
    pdf_deque_pop_end(deque, &node);
    double dx1 = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dy1 = *((double*)data);
    float curx = 0, cury = 0;
    // plutovg_canvas_get_current_point(canvas, &curx, &cury);
    plutovg_canvas_move_to(canvas, curx + dx1, cury + dy1);
    pdf_deque_empty(deque);
}
void handle_hmoveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    if (deque->size > 1)
    {
        pdf_deque_pop_end(deque, &node);
    }
    float x = 0, y = 0;
    //plutovg_canvas_get_current_point(canvas, &x, &y);
    pdf_deque_pop_end(deque, &node);
    double dx1 = *((double*)data);
    plutovg_canvas_move_to(canvas, dx1, 0);
    pdf_deque_empty(deque);
}
void handle_vstemhm(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    printf("vstemhm not implemented\n");
    if (deque->size % 2 != 0)
    {
        pdf_deque_pop_end(deque, &node);
    }
    pdf_deque_pop_end(deque, &node);
    double x = *((double*)data);
    pdf_deque_pop_end(deque, &node);
    double dx = *((double*)data);
    while (deque->size > 0 && deque->size % 2 == 0)
    {
        pdf_deque_pop_end(deque, &node);
        double dxa = *((double*)data);
        pdf_deque_pop_end(deque, &node);
        double dxb = *((double*)data);
    }
    pdf_deque_empty(deque);
}
void handle_rcurveline(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = 0, cury = 0;
    // plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size > 0 && deque->size % 6 == 0)
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

    plutovg_canvas_line_to(canvas, curx + dxd, cury + dyd);

    pdf_deque_empty(deque);
}
void handle_rlinecurve(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float curx = 0, cury = 0;
    // plutovg_canvas_get_current_point(canvas, &curx, &cury);
    while (deque->size > 0 && deque->size % 6 == 0)
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

    pdf_deque_empty(deque);
}
void handle_vvcurveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = 0, cur_y = 0;
    // plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    double dx1;
    if (deque->size % 4 == 1)
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
    pdf_deque_empty(deque);
}
void handle_hhcurveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    float cur_x = 0, cur_y = 0;
    //plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    double dy1;
    if (deque->size % 4 == 1)
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
    pdf_deque_empty(deque);
}
void handle_callgsubr(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    pdf_array_t* global_subr_index = context->state->textState.font->global_subr;
    uint16_t global_bias = context->state->textState.font->global_subr_bias;

    pdf_deque_pop_front(deque, &node);
                    double g = *((double*)data);
                    uint32_t off = g + global_bias;
                    _cff_do_render_char(context, deque, 
                        global_subr_index->values[off]->val.string, 
                        global_subr_index->values[off]->value_len);
}
void handle_vhcurveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    bool last_dyf = deque->size % 8 == 4 || deque->size % 8 == 5;
    bool is_vertical_start = true;
    float cur_x = 0, cur_y = 0;
    //plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    if (deque->size % 4 == 0)
    {
        while (deque->size >= 4)
        {
            double params[4];
            for (int i = 0; i < 4; i++)
            {
                pdf_deque_pop_end(deque, &node);
                params[i] = *((double*)data);
            }
            if (is_vertical_start)
            {
                double x1 = cur_x;
                double y1 = cur_y + params[0];
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2 + params[3];
                double y3 = y2;

                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            else
            {
                double x1 = cur_x + params[0];
                double y1 = cur_y;
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2;
                double y3 = y2 + params[3];
                
                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            is_vertical_start = !is_vertical_start;
        }  
    }
    else
    {
        while (deque->size > 5)
        {
            double params[4];
            for (int i = 0; i < 4; i++)
            {
                pdf_deque_pop_end(deque, &node);
                params[i] = *((double*)data);
            }
            if (is_vertical_start)
            {
                double x1 = cur_x;
                double y1 = cur_y + params[0];
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2 + params[3];
                double y3 = y2;

                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            else
            {
                double x1 = cur_x + params[0];
                double y1 = cur_y;
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2;
                double y3 = y2 + params[3];
                
                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            is_vertical_start = !is_vertical_start;
        }
        double params[5];
        for (int i = 0; i < 5; i++)
        {
            pdf_deque_pop_end(deque, &node);
            params[i] = *((double*)data);
        }
        double x1 = cur_x;
        double y1 = cur_y + params[0];
        double x2 = x1 + params[1];
        double y2 = y1 + params[2];
        if (last_dyf)
        {
            double x3 = x2 + params[3];
            double y3 = y2 + params[4];
            plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        }
        else
        {
            double x3 = x2 + params[4];
            double y3 = y2 + params[3];
            plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        }
    }
    pdf_deque_empty(deque);
}
void handle_hvcurveto(pdf_context_t* context, pdf_deque_t* deque)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    plutovg_canvas_t* canvas = context->canvas;
    // z = 4 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    // z = 8 + 8x + y (x = 0,1,2,3...,y= 0 or 1)
    bool last_dyf = deque->size % 8 == 0 || deque->size % 8 == 1;
    bool is_horizontal_start = true;
    float cur_x = 0, cur_y = 0;
    //plutovg_canvas_get_current_point(canvas, &cur_x, &cur_y);
    if (deque->size % 4 == 0)
    {
        while (deque->size >= 4)
        {
            double params[4];
            for (int i = 0; i < 4; i++)
            {
                pdf_deque_pop_end(deque, &node);
                params[i] = *((double*)data);
            }
            if (is_horizontal_start)
            {
                double x1 = cur_x + params[0];
                double y1 = cur_y;
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2;
                double y3 = y2 + params[3];

                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            else
            {
                double x1 = cur_x;
                double y1 = cur_y + params[0];
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2 + params[3];
                double y3 = y2;
                
                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            is_horizontal_start = !is_horizontal_start;
        }  
    }
    else
    {
        while (deque->size > 5)
        {
            double params[4];
            for (int i = 0; i < 4; i++)
            {
                pdf_deque_pop_end(deque, &node);
                params[i] = *((double*)data);
            }
            if (is_horizontal_start)
            {
                double x1 = cur_x + params[0];
                double y1 = cur_y;
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2;
                double y3 = y2 + params[3];

                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            else
            {
                double x1 = cur_x;
                double y1 = cur_y + params[0];
                double x2 = x1 + params[1];
                double y2 = y1 + params[2];
                double x3 = x2 + params[3];
                double y3 = y2;
                
                plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
                cur_x = x3;
                cur_y = y3;
            }
            is_horizontal_start = !is_horizontal_start;
        }
        double params[5];
        for (int i = 0; i < 5; i++)
        {
            pdf_deque_pop_end(deque, &node);
            params[i] = *((double*)data);
        }
        double x1 = cur_x + params[0];
        double y1 = cur_y;
        double x2 = x1 + params[1];
        double y2 = y1 + params[2];
        if (last_dyf)
        {
            double x3 = x2 + params[3];
            double y3 = y2 + params[4];

            plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        }
        else
        {
            double x3 = x2 + params[4];
            double y3 = y2 + params[3];

            plutovg_canvas_cubic_to(canvas, x1, y1, x2, y2, x3, y3);
        }
        
    }
    pdf_deque_empty(deque);
}

void handle_and(pdf_context_t* context, pdf_deque_t* deque)
{printf("and not implemented\n");}
void handle_or(pdf_context_t* context, pdf_deque_t* deque)
{printf("or not implemented\n");}
void handle_not(pdf_context_t* context, pdf_deque_t* deque)
{ printf("not not implemented\n");}
void handle_abs(pdf_context_t* context, pdf_deque_t* deque)
{printf("abs not implemented\n");}
void handle_add(pdf_context_t* context, pdf_deque_t* deque)
{printf("add not implemented\n");}
void handle_sub(pdf_context_t* context, pdf_deque_t* deque)
{printf("sub not implemented\n");}
void handle_div(pdf_context_t* context, pdf_deque_t* deque)
{printf("div not implemented\n");}
void handle_neg(pdf_context_t* context, pdf_deque_t* deque)
{printf("neg not implemented\n");}
void handle_eq(pdf_context_t* context, pdf_deque_t* deque)
{printf("eq not implemented\n");}
void handle_drop(pdf_context_t* context, pdf_deque_t* deque)
{printf("drop not implemented\n");}
void handle_put(pdf_context_t* context, pdf_deque_t* deque)
{printf("put not implemented\n");}
void handle_get(pdf_context_t* context, pdf_deque_t* deque)
{printf("get not implemented\n");}
void handle_ifelse(pdf_context_t* context, pdf_deque_t* deque)
{printf("ifelse not implemented\n");}
void handle_random(pdf_context_t* context, pdf_deque_t* deque)
{printf("random not implemented\n");}
void handle_mul(pdf_context_t* context, pdf_deque_t* deque)
{printf("mul not implemented\n");}
void handle_sqrt(pdf_context_t* context, pdf_deque_t* deque)
{printf("sqrt not implemented\n");}
void handle_dup(pdf_context_t* context, pdf_deque_t* deque)
{printf("dup not implemented\n");}
void handle_exch(pdf_context_t* context, pdf_deque_t* deque)
{printf("exch not implemented\n");}
void handle_index(pdf_context_t* context, pdf_deque_t* deque)
{printf("index not implemented\n");}
void handle_roll(pdf_context_t* context, pdf_deque_t* deque)
{printf("roll not implemented\n");}
void handle_hflex(pdf_context_t* context, pdf_deque_t* deque)
{printf("hflex not implemented\n");}
void handle_flex(pdf_context_t* context, pdf_deque_t* deque)
{printf("flex not implemented\n");}
void handle_hflex1(pdf_context_t* context, pdf_deque_t* deque)
{printf("hflex1 not implemented\n");}
void handle_flex1(pdf_context_t* context, pdf_deque_t* deque)
{printf("flex1 not implemented\n");}
