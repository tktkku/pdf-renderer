#include "pdf-private.h"
#include "pdf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#define STB_IMAGE_IMPLEMENTATION
#include "plutovg-stb-image-write.h"
#include "plutovg-stb-image.h"

typedef void (*OPERATION_HANDLER)(pdf_context_t* context);

typedef struct {
    char* operation;
    OPERATION_HANDLER handler;
} handler_entry;

void handle_q(pdf_context_t* context);
void handle_Q(pdf_context_t* context);
void handle_cm(pdf_context_t* context);
void handle_w(pdf_context_t* context);
void handle_J(pdf_context_t* context);
void handle_j(pdf_context_t* context);
void handle_M(pdf_context_t* context);
void handle_d(pdf_context_t* context);
void handle_ri(pdf_context_t* context);
void handle_i(pdf_context_t* context);
void handle_gs(pdf_context_t* context);
void handle_m(pdf_context_t* context);
void handle_l(pdf_context_t* context);
void handle_c(pdf_context_t* context);
void handle_v(pdf_context_t* context);
void handle_y(pdf_context_t* context);
void handle_h(pdf_context_t* context);
void handle_re(pdf_context_t* context);
void handle_S(pdf_context_t* context);
void handle_s(pdf_context_t* context);
void handle_F_f(pdf_context_t* context);
void handle_f_star(pdf_context_t* context);
void handle_B(pdf_context_t* context);
void handle_B_star(pdf_context_t* context);
void handle_b(pdf_context_t* context);
void handle_b_star(pdf_context_t* context);
void handle_n(pdf_context_t* context);
void handle_W(pdf_context_t* context);
void handle_W_star(pdf_context_t* context);
void handle_CS(pdf_context_t* context);
void handle_SC(pdf_context_t* context);
void handle_G(pdf_context_t* context);
void handle_cs(pdf_context_t* context);
void handle_sc(pdf_context_t* context);
void handle_g(pdf_context_t* context);
void handle_RG(pdf_context_t* context);
void handle_rg(pdf_context_t* context);
void handle_K(pdf_context_t* context);
void handle_k(pdf_context_t* context);
void handle_SCN(pdf_context_t* context);
void handle_scn(pdf_context_t* context);
void handle_sh(pdf_context_t* context);
void handle_Do(pdf_context_t* context);
void handle_BI(pdf_context_t* context);
void handle_ID(pdf_context_t* context);
void handle_EI(pdf_context_t* context);
void handle_BT(pdf_context_t* context);
void handle_ET(pdf_context_t* context);
void handle_Tf(pdf_context_t* context);
void handle_Tc(pdf_context_t* context);
void handle_Tw(pdf_context_t* context);
void handle_Tz(pdf_context_t* context);
void handle_TL(pdf_context_t* context);
void handle_Tr(pdf_context_t* context);
void handle_Ts(pdf_context_t* context);
void handle_Td(pdf_context_t* context);
void handle_TD(pdf_context_t* context);
void handle_Tm(pdf_context_t* context);
void handle_T_star(pdf_context_t* context);
void handle_Tj(pdf_context_t* context);
void handle_apostrophe(pdf_context_t* context);
void handle_quotation(pdf_context_t* context);
void handle_TJ(pdf_context_t* context);
void handle_d0(pdf_context_t* context);
void handle_d1(pdf_context_t* context);
void handle_BDC(pdf_context_t* context);
void handle_BMC(pdf_context_t* context);
void handle_DP(pdf_context_t* context);
void handle_EMC(pdf_context_t* context);
void handle_MP(pdf_context_t* context);

const static handler_entry handlers[] = {
    {"\"", handle_quotation}, {"'", handle_apostrophe}, {"B", handle_B},
    {"B*", handle_B_star},    {"BDC", handle_BDC},      {"BMC", handle_BMC},
    {"BI", handle_BI},        {"BT", handle_BT},        {"CS", handle_CS},
    {"DP", handle_DP},        {"Do", handle_Do},        {"EI", handle_EI},
    {"EMC", handle_EMC},      {"ET", handle_ET},        {"F", handle_F_f},
    {"G", handle_G},          {"ID", handle_ID},        {"J", handle_J},
    {"K", handle_K},          {"M", handle_M},          {"MP", handle_MP},
    {"Q", handle_Q},          {"RG", handle_RG},        {"S", handle_S},
    {"SC", handle_SC},        {"SCN", handle_SCN},      {"T*", handle_T_star},
    {"TD", handle_TD},        {"TJ", handle_TJ},        {"TL", handle_TL},
    {"Tc", handle_Tc},        {"Td", handle_Td},        {"Tf", handle_Tf},
    {"Tj", handle_Tj},        {"Tm", handle_Tm},        {"Tr", handle_Tr},
    {"Ts", handle_Ts},        {"Tw", handle_Tw},        {"Tz", handle_Tz},
    {"W", handle_W},          {"W*", handle_W_star},    {"b", handle_b},
    {"b*", handle_b_star},    {"c", handle_c},          {"cm", handle_cm},
    {"cs", handle_cs},        {"d", handle_d},          {"d0", handle_d0},
    {"d1", handle_d1},        {"f", handle_F_f},        {"f*", handle_f_star},
    {"g", handle_g},          {"gs", handle_gs},        {"h", handle_h},
    {"i", handle_i},          {"j", handle_j},          {"k", handle_k},
    {"l", handle_l},          {"m", handle_m},          {"n", handle_n},
    {"q", handle_q},          {"re", handle_re},        {"rg", handle_rg},
    {"ri", handle_ri},        {"s", handle_s},          {"sc", handle_sc},
    {"scn", handle_scn},      {"sh", handle_sh},        {"v", handle_v},
    {"w", handle_w},          {"y", handle_y} };
void _do_render_operation(pdf_context_t* context, const char* operation);
void render_to_png_by_plutovg(pdf_page_t* page, char* filename)
{
    int width = pdf_page_get_media_width(page) * PIXELS_PER_POINT;
    int height = pdf_page_get_media_height(page) * PIXELS_PER_POINT;
    int stride = width * 4;
    unsigned char* pixels = (unsigned char*)malloc(stride * height);
    memset(pixels, 0xFF, stride * height);
    pdf_stack_t* stack = pdf_stack_init();

    pdf_context_t context;

    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels, width, height, stride);
    // plutovg_surface_t* surface = plutovg_surface_create(width, height);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 0.5);
    plutovg_canvas_set_miter_limit(canvas, 10.0);

    plutovg_matrix_init(&context.textState.fontMatrixPlutovg, 0, 0, 0, 0, 0, 0);

    plutovg_canvas_restore(canvas);

    // Flip the Y-axis
    plutovg_canvas_translate(canvas, 0, height);
    plutovg_canvas_scale(canvas, PIXELS_PER_POINT, -PIXELS_PER_POINT);
    context.canvas = canvas;
    context.graphicsState.strokeColor[0] = 0;
    context.graphicsState.strokeColor[1] = 0;
    context.graphicsState.strokeColor[2] = 0;
    context.graphicsState.fillColor[0] = 0;
    context.graphicsState.fillColor[1] = 0;
    context.graphicsState.fillColor[2] = 0;
    context.stack = stack;
    context.pdf = page->pdf;
    context.page = page;
    context.textState.textMode = 0;
    context.textState.textLeading = 0;
    context.textState.font = NULL;
    context.textState.fontface = NULL;
    int numStreams = pdf_page_get_streams(page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(page, j);
        if (stream == NULL)
            continue;
        pdf_stream_open(stream);
        pdf_parser_token_t* tk;
        int count = 0;
        while ((tk = pdf_stream_get_next_token(stream)) != NULL)
        {
            const char* token = pdf_parser_token_get_token(tk);
            if (token == NULL)
                continue;
            // printf("%s\n", token);
            _do_render_operation(&context, token);
            // if (strcmp(token, "Tj") == 0)
            // {
            //     plutovg_surface_write_to_png(surface, "test.png");
            //     printf("Press any key to continue...");
            //     getchar();
            // }
            pdf_parser_token_free(tk);
        }

        pdf_stream_close(stream);
    }
    pdf_stack_free(stack);
    plutovg_surface_write_to_png(surface, filename);
    if (context.textState.fontface != NULL)
    {
        plutovg_canvas_set_font_face(context.canvas, context.textState.fontface);
        plutovg_font_face_destroy(context.textState.fontface);
    }
    if (context.textState.font != NULL)
    {
        pdf_font_free(context.textState.font);
    }
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
    free(pixels);
}

void render_to_buffer_by_plutovg(pdf_page_t* page, unsigned char* pixels,
    int width, int height, int stride)
{
    pdf_stack_t* stack = pdf_stack_init();

    pdf_context_t context;

    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels, width, height, stride);
    // plutovg_surface_t* surface = plutovg_surface_create(width, height);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 0.5);
    plutovg_canvas_set_miter_limit(canvas, 10.0);

    plutovg_matrix_init(&context.textState.fontMatrixPlutovg, 0, 0, 0, 0, 0, 0);

    plutovg_canvas_restore(canvas);

    // Flip the Y-axis
    plutovg_canvas_translate(canvas, 0, height);
    plutovg_canvas_scale(canvas, PIXELS_PER_POINT, -PIXELS_PER_POINT);
    context.canvas = canvas;
    context.graphicsState.strokeColor[0] = 0;
    context.graphicsState.strokeColor[1] = 0;
    context.graphicsState.strokeColor[2] = 0;
    context.graphicsState.fillColor[0] = 0;
    context.graphicsState.fillColor[1] = 0;
    context.graphicsState.fillColor[2] = 0;
    context.stack = stack;
    context.pdf = page->pdf;
    context.page = page;
    context.textState.textMode = 0;
    context.textState.textLeading = 0;
    context.textState.font = NULL;
    context.textState.fontface = NULL;
    int numStreams = pdf_page_get_streams(page);
    for (int j = 0; j < numStreams; j++)
    {
        pdf_stream_t* stream = pdf_page_get_stream(page, j);
        if (stream == NULL)
            continue;
        pdf_stream_open(stream);
        pdf_parser_token_t* tk;
        int count = 0;
        while ((tk = pdf_stream_get_next_token(stream)) != NULL)
        {
            const char* token = pdf_parser_token_get_token(tk);
            if (token == NULL)
                continue;
            // printf("%s\n", token);
            _do_render_operation(&context, token);
            // if (strcmp(token, "f") == 0)
            // {
            //     plutovg_surface_write_to_png(surface, "test.png");
            //     printf("Press any key to continue...");
            //     getchar();
            // }
            pdf_parser_token_free(tk);
        }

        pdf_stream_close(stream);
    }
    pdf_stack_free(stack);
    if (context.textState.fontface != NULL)
    {
        plutovg_canvas_set_font_face(context.canvas, context.textState.fontface);
        plutovg_font_face_destroy(context.textState.fontface);
    }
    if (context.textState.font != NULL)
    {
        pdf_font_free(context.textState.font);
    }
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);
}

void _do_render_operation(pdf_context_t* context, const char* operation)
{
    printf("%s\n", operation);
    int count = ARRAY_COUNT(handlers);
    int left = 0;
    int right = count - 1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        int cmp = strcmp(handlers[mid].operation, operation);

        if (cmp == 0)
        {
            if (handlers[mid].handler != NULL)
            {
                if (strchr("fFbBW", operation[0]) != NULL)
                {
                    if (operation[1] == '\0')
                        plutovg_canvas_set_fill_rule(context->canvas,
                            PLUTOVG_FILL_RULE_NON_ZERO);
                    else if (operation[1] == '*')
                        plutovg_canvas_set_fill_rule(context->canvas,
                            PLUTOVG_FILL_RULE_EVEN_ODD);
                }
                handlers[mid].handler(context);
                return;
            }
        }
        else if (cmp < 0)
        {
            left = mid + 1;
        }
        else
        {
            right = mid - 1;
        }
    }

    // did not match any operation
    // push data to stack
    pdf_stack_push(context->stack, operation, strlen(operation) + 1);
}
void stroke(pdf_context_t* context)
{
    double r, g, b, a;
    plutovg_color_t color;
    plutovg_canvas_get_color(context->canvas, &color);
    plutovg_canvas_set_rgb(context->canvas, context->graphicsState.strokeColor[0],
        context->graphicsState.strokeColor[1], context->graphicsState.strokeColor[2]);
    plutovg_canvas_stroke(context->canvas);
    plutovg_canvas_set_color(context->canvas, &color);
    // plutovg_canvas_set_rgb(context->canvas,
    //     context->fillColor[0],
    //     context->fillColor[1],
    //     context->fillColor[2]);
}
void handle_q(pdf_context_t* context)
{
    // store state
    plutovg_canvas_save(context->canvas);
}

void handle_Q(pdf_context_t* context)
{
    // restore state
    plutovg_canvas_restore(context->canvas);
}

void handle_cm(pdf_context_t* context)
{
    // change matrix CTM
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float a = strtof(buf, NULL);

    plutovg_matrix_t ctm;
    plutovg_matrix_init(&ctm, a, b, c, d, e, f);
    plutovg_canvas_transform(context->canvas, &ctm);
}

void handle_w(pdf_context_t* context)
{
    // set line width
    // lineWidth
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    plutovg_canvas_set_line_width(context->canvas, w);
}

void handle_J(pdf_context_t* context)
{
    // set cap style
    // lineCap
    pdf_stack_node_t node;
    char buf[1024] = { 0 };
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int c = atoi(buf);

    plutovg_canvas_set_line_cap(context->canvas, c);
}

void handle_j(pdf_context_t* context)
{
    // set join style
    // lineJoin

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int j = atoi(buf);

    plutovg_canvas_set_line_join(context->canvas, j);
}

void handle_M(pdf_context_t* context)
{
    // set miter limit
    // miterLimit
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    plutovg_canvas_set_miter_limit(context->canvas, m);
}

void handle_d(pdf_context_t* context)
{
    // set line dash pattern
    // dashArray dashPhase
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float offset = strtof(buf, NULL);

    int index = 2;
    float dashs[2] = { 0 };
    while (true)
    {
        pdf_stack_pop(context->stack, &node);
        if (!strcmp(node.data, "]"))
        {
            // ignore
        }
        else if (!strcmp(node.data, "["))
        {
            break;
        }
        else
        {
            float dash = strtof(buf, NULL);
            if (index > 0)
                dashs[--index] = dash;
        }
    }

    plutovg_canvas_set_dash_offset(context->canvas, offset);
    plutovg_canvas_set_dash_array(context->canvas, dashs, 2);
}

void handle_ri(pdf_context_t* context)
{
    // set color rendering intent
    // intent
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}

void handle_i(pdf_context_t* context)
{
    // set flatness tolerance
    // flatness
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float i = strtof(buf, NULL);
    // context->graphics_state.flatness = i;
}

void handle_gs(pdf_context_t* context)
{
    // set specified parameters
    // dictName shall be the name of
    // a graphics state parameter dictionary
    // in the ExtGState subdictionary of the current resource dictionary
    // dictName
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_page_get_ext_gstate(context->page, buf);
}

void handle_m(pdf_context_t* context)
{
    // begin a new subpath by moving the current point to (x,y)
    // x y
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_move_to(context->canvas, x, y);
}

void handle_l(pdf_context_t* context)
{
    // append a straight line segment from the current point to (x, y)
    // x y
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);
    plutovg_canvas_line_to(context->canvas, x, y);
}

void handle_c(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x2 ,y2) as the Bezier control points
    // x1 y1 x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y1 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}

void handle_v(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current point
    // the curve shall extend from the current point to (x3 ,y3)
    // using the current point and (x2, y2) as the Bezier control points
    // x1 y1 same as current point
    // x2 y2 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float x1, y1, x2, y2, x3, y3;
    pdf_stack_pop(context->stack, &node);
    y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    y2 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    x2 = strtof(buf, NULL);

    plutovg_canvas_get_current_point(context->canvas, &x1, &y1);
    plutovg_canvas_cubic_to(context->canvas, x1, y1, x2, y2, x3, y3);
}

void handle_y(pdf_context_t* context)
{
    // append a cubic Bezier curve to the current path
    // the curve shall extend from the current point to (x3, y3)
    // using (x1, y1) and (x3, y3) as the Bezier control points
    // x2 y2 same as x3 y3
    // x1 y1 x3 y3
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float y3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x3 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y1 = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x1 = strtof(buf, NULL);

    plutovg_canvas_cubic_to(context->canvas, x1, y1, x3, y3, x3, y3);
}

void handle_h(pdf_context_t* context)
{
    // close the current subpath by appending a straight line segment
    // from the current point to the starting point of the subpath
    // if the current subpath is already closed, do nothing
    // this operator terminates the current subpath
    plutovg_canvas_close_path(context->canvas);
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
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float height = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float width = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float x = strtof(buf, NULL);

    plutovg_canvas_rect(context->canvas, x, y, width, height);
}

void handle_S(pdf_context_t* context)
{
    // stroke the path
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
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

void handle_F_f(pdf_context_t* context)
{
    // fill the path, using the nonzero winding number rule
    // to determine the region to fill

    plutovg_canvas_set_rgb(context->canvas, context->graphicsState.fillColor[0],
        context->graphicsState.fillColor[1], context->graphicsState.fillColor[2]);
    plutovg_canvas_fill(context->canvas);
}

void handle_f_star(pdf_context_t* context)
{
    // fill the path, using the even-odd rule
    // to determine the region to fill
    plutovg_canvas_fill(context->canvas);
}

void handle_B(pdf_context_t* context)
{
    // fill and then stroke the path, using the nonzero winding number rule
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_B_star(pdf_context_t* context)
{
    // fill and then stroke the path, using the even-odd rule
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_b(pdf_context_t* context)
{
    // close fill and then stroke the path, using nonzero winding number rule
    // same as the sequence
    // h B
    plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_b_star(pdf_context_t* context)
{
    // close fill and then stroke the path using the even-odd rule
    // same as the sequence
    // h B*
    plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_fill(context->canvas);
    // plutovg_canvas_stroke(context->canvas);
    stroke(context);
}

void handle_n(pdf_context_t* context)
{
    // end the path object without filling or stroking it
    // plutovg_canvas_close_path(context->canvas);
    plutovg_canvas_new_path(context->canvas);
}

void handle_W(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the current path
    // using nonzero winding number rule

    plutovg_canvas_clip(context->canvas);
}

void handle_W_star(pdf_context_t* context)
{
    // modify the current clipping path by intersecting it with the curerent path
    // using the even-odd rule

    plutovg_canvas_clip(context->canvas);
}

void handle_CS(pdf_context_t* context)
{
    // set color space
    // /DeviceGray
    // initialize the corresponding current color to 0.0
    /// DeviceRGB
    // initialize the corresponding current color of red green blue to 0.0
    // /DeviceCMYK
    // initialize the corresponding current color of cyan magenta yellow to 0.0
    // and the black to 1.0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    memcpy(context->graphicsState.currentColorSpace, buf, strlen(buf) + 1);
}

void handle_SC(pdf_context_t* context)
{
    // set gray level, 0.0 to balck 1.0 to white

    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;

    if (!strcmp(context->graphicsState.currentColorSpace, "/DeviceGray"))
    {
        // gray
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, g, g, g);
        context->graphicsState.strokeColor[0] = g;
        context->graphicsState.strokeColor[1] = g;
        context->graphicsState.strokeColor[2] = g;
    }
    else if (!strcmp(context->graphicsState.currentColorSpace,
        "/DeviceRGB"))
    {
        // red green blue

        pdf_stack_pop(context->stack, &node);
        float b = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float g = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float r = strtof(buf, NULL);
        // plutovg_canvas_set_rgb(context->canvas, r, g, b);
        context->graphicsState.strokeColor[0] = r;
        context->graphicsState.strokeColor[1] = g;
        context->graphicsState.strokeColor[2] = b;
    }
    else if (!strcmp(context->graphicsState.currentColorSpace,
        "/DeviceCMYK"))
    {
        // cyan magenta yellow black
        pdf_stack_pop(context->stack, &node);
        float k = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float y = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float m = strtof(buf, NULL);
        pdf_stack_pop(context->stack, &node);
        float c = strtof(buf, NULL);

        float r = (1.0 - c) * (1.0 - k);
        float g = (1.0 - m) * (1.0 - k);
        float b = (1.0 - y) * (1.0 - k);

        context->graphicsState.strokeColor[0] = r;
        context->graphicsState.strokeColor[1] = g;
        context->graphicsState.strokeColor[2] = b;
    }
}

void handle_G(pdf_context_t* context)
{
    // set both in one operation
    // gray
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    context->graphicsState.strokeColor[0] = g;
    context->graphicsState.strokeColor[1] = g;
    context->graphicsState.strokeColor[2] = g;
    strcpy(context->graphicsState.currentColorSpace, "/DeviceGray");
}

void handle_cs(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // stack_node_t node;
    // node.data = buf;
    // stack_pop(context->stack, &node);

    handle_CS(context);
}

void handle_sc(pdf_context_t* context)
{
    // for nonstroking
    // char buf[1024] = { 0 };
    // stack_node_t node;
    // node.data = buf;
    // stack_pop(context->stack, &node);
    handle_SC(context);
}

void handle_g(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    // handle_G(context);
    plutovg_canvas_set_rgb(context->canvas, g, g, g);
    context->graphicsState.fillColor[0] = g;
    context->graphicsState.fillColor[1] = g;
    context->graphicsState.fillColor[2] = g;
}

void handle_RG(pdf_context_t* context)
{
    // combine CS and SC for DeviceRGB
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);
    context->graphicsState.strokeColor[0] = r;
    context->graphicsState.strokeColor[1] = g;
    context->graphicsState.strokeColor[2] = b;
    // plutovg_canvas_set_rgb(context->canvas, gray, gray, gray);
    strcpy(context->graphicsState.currentColorSpace, "/DeviceRGB");
}

void handle_rg(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float g = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);
    plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->graphicsState.fillColor[0] = r;
    context->graphicsState.fillColor[1] = g;
    context->graphicsState.fillColor[2] = b;
    // handle_RG(context);
}

void handle_K(pdf_context_t* context)
{
    // combine CS and SC for DeviceCMYK
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float k = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->graphicsState.strokeColor[0] = (1.0 - c) * (1.0 - k);
    context->graphicsState.strokeColor[1] = (1.0 - m) * (1.0 - k);
    context->graphicsState.strokeColor[2] = (1.0 - y) * (1.0 - k);
    strcpy(context->graphicsState.currentColorSpace, "/DeviceCMYK");
}

void handle_k(pdf_context_t* context)
{
    // for nonstroking
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float k = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float y = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float m = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    // handle_K(context);
    float r = (1.0 - c) * (1.0 - k);
    float g = (1.0 - m) * (1.0 - k);
    float b = (1.0 - y) * (1.0 - k);
    plutovg_canvas_set_rgb(context->canvas, r, g, b);
    context->graphicsState.fillColor[0] = r;
    context->graphicsState.fillColor[1] = g;
    context->graphicsState.fillColor[2] = b;
}

void handle_SCN(pdf_context_t* context)
{
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_scn(pdf_context_t* context)
{
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_sh(pdf_context_t* context)
{
    // name
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}

void handle_Do(pdf_context_t* context)
{
    // paint a specified XObject
    // name
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);

    pdf_xobject_t* xobj = pdf_page_get_xobject(context->page, buf);
    if (xobj == NULL)
    {
        return;
    }
    plutovg_canvas_save(context->canvas);
    if (xobj->type == XOBJ_FORM)
    {
        plutovg_matrix_t m;
        plutovg_matrix_init(&m, xobj->form->matrix[0], xobj->form->matrix[1],
            xobj->form->matrix[2], xobj->form->matrix[3],
            xobj->form->matrix[4], xobj->form->matrix[5]);
        plutovg_canvas_transform(context->canvas, &m);
        plutovg_canvas_rect(context->canvas, xobj->form->bbox[0],
            xobj->form->bbox[1], xobj->form->bbox[2],
            xobj->form->bbox[3]);
        plutovg_canvas_clip(context->canvas);

    }
    else if (xobj->type == XOBJ_IMAGE)
    {
        char filename[1280] = { 0 };
        int width = xobj->image->width;
        int height = xobj->image->height;
        int channels = 3;
        if (!strcmp(xobj->image->color_space, "/DeviceGray"))
        {
            channels = 1;
        }
        if (xobj->image->data_len == 0)
        {
            return;
        }
        plutovg_surface_t* s = NULL;
        if (memcmp(xobj->image->data, "\xFF\xD8", 2) == 0) // jpeg
        {
            s = plutovg_surface_load_from_image_data(xobj->image->data,
                xobj->image->data_len);
            // unsigned char* pixels = stbi_load_from_memory(xobj->image->data,
            // xobj->image->data_len, &width, &height, &channels, channels);
            // //convert_to_gray(pixels, width, height, width * channels, channels);
            // //average_gray(pixels, width, height, width * channels, channels);
            // //dither_by_threshold(pixels, width, height, width * channels,
            // channels, 220); free(xobj->image->data); xobj->image->data = pixels;
            // xobj->image->data_len = width * channels * height;
            // dither_by_threshold(pixels, width, height, width * channels, 220);
            // stbi_image_free(pixels);
        }
        else if (memcmp(xobj->image->data, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",
            8) == 0) // png
        {
            s = plutovg_surface_load_from_image_data(xobj->image->data,
                xobj->image->data_len);
        }
        else if (memcmp(xobj->image->data, "P6", 2) == 0) // ppm
        {
            s = plutovg_surface_load_from_image_data(xobj->image->data,
                xobj->image->data_len);
        }
        else
        {
            // tje_encode_to_file("tje.jpg", width, height, channels,
            // xobj->image->data);
            //  add header
            char header[128] = { 0 };
            if (channels == 3)
            {
                sprintf(header, "P6 %d %d 255\n", width, height);
            }
            else if (channels == 1)
            {
                sprintf(header, "P5 %d %d 255\n", width, height);
            }

            int header_len = strlen(header);
            int actual_line_bytes = xobj->image->data_len / xobj->image->height;
            int actual_data_len = xobj->image->data_len;
            int real_line_bytes = channels * xobj->image->width;
            int real_data_len = real_line_bytes * xobj->image->height;
            if (channels == 3 && real_data_len != actual_data_len)
            {
                unsigned char* tmp =
                    (unsigned char*)malloc(real_data_len + header_len);
                int off = 0;
                memcpy(tmp, header, header_len);
                off += header_len;
                for (int i = 0; i < xobj->image->height; i++)
                {
                    memcpy(tmp + off, xobj->image->data + i * actual_line_bytes,
                        real_line_bytes);
                    off += real_line_bytes;
                }
                free(xobj->image->data);
                xobj->image->data = tmp;
                xobj->image->data_len = off;
            }
            else
            {
                unsigned char* tmp =
                    (unsigned char*)malloc(xobj->image->data_len + header_len);
                int off = 0;
                memcpy(tmp, header, header_len);
                off += header_len;
                memcpy(tmp + off, xobj->image->data, xobj->image->data_len);
                off += xobj->image->data_len;
                free(xobj->image->data);
                xobj->image->data = tmp;
                xobj->image->data_len = off;
            }

            s = plutovg_surface_load_from_image_data(xobj->image->data,
                xobj->image->data_len);
        }
        if (channels == 3)
        {
            // convert_to_gray(xobj->image->data, width, height, width * channels,
            // channels); otsu(xobj->image->data, width, height, width * channels,
            // channels);
        }

        if (s == NULL)
        {
            return;
        }

        sprintf(filename, "%s.png", buf + 1);
        plutovg_surface_write_to_png(s, filename);

        // Scale factors to normalize image dimensions to unit space
        // plutovg_matrix_t m = { xobj->image->width, 0, 0, -xobj->image->height, 0,
        // xobj->image->height }; plutovg_matrix_invert(&m, &m);
        // plutovg_canvas_set_texture(context->canvas, s,
        // PLUTOVG_TEXTURE_TYPE_PLAIN, 1.f, &m);
        float scale_x = 1.f / xobj->image->width;
        float scale_y = 1.f / xobj->image->height;

        // Transformation matrix to scale and flip the image vertically
        plutovg_matrix_t m = { scale_x,  0, 0,
                              -scale_y, 0, xobj->image->height * scale_y };

        plutovg_canvas_set_texture(context->canvas, s, PLUTOVG_TEXTURE_TYPE_PLAIN,
            0.9f, &m);
        plutovg_canvas_paint(context->canvas);
        plutovg_surface_destroy(s);
    }
    // pdf_page_xobject_free(xobj);
    plutovg_canvas_restore(context->canvas);
}

void handle_BI(pdf_context_t* context)
{
    // begin an inline image object
}

void handle_ID(pdf_context_t* context)
{
    // begin the image data for an inline image object
}

void handle_EI(pdf_context_t* context)
{
    // end an inline image object
}

void handle_BT(pdf_context_t* context)
{
    // begin text
    plutovg_canvas_save(context->canvas);
    //plutovg_canvas_move_to(context->canvas, 0, 0);
    // context->fontface = NULL;
    // context->font = NULL;
    context->textState.lineWidth = 0;
}

void handle_ET(pdf_context_t* context)
{
    // end text
    plutovg_canvas_restore(context->canvas);
    // if (context->fontface)
    // {
    //     plutovg_canvas_set_font_face(context->canvas, NULL);
    //     plutovg_font_face_destroy(context->fontface);
    // }
    // if (context->font)
    // {
    //     pdf_font_free(context->font);
    //     context->font = NULL;
    // }
}

void handle_Tc(pdf_context_t* context)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float c = strtof(buf, NULL);
    context->textState.characterSpacing = c;
}

void handle_Tw(pdf_context_t* context)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float w = strtof(buf, NULL);
    context->textState.wordSpacing = w;
}

void handle_Tz(pdf_context_t* context)
{
    // horizontal scaling
    // scale initial value=100
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float h = strtof(buf, NULL);
    plutovg_canvas_scale(context->canvas, h / 100.0, 1.0);
    context->textState.horizontalScaling = h;
}

void handle_TL(pdf_context_t* context)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float t = strtof(buf, NULL);
    context->textState.textLeading = t;
}

void handle_Tf(pdf_context_t* context)
{
    // set font and font size to use
    // fontname fontsize
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float fontsize = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    pdf_font_t* font = pdf_page_get_font(context->page, buf);
    context->textState.fontSize = fontsize;
    if (font == NULL)
        return;
    else
    {
        pdf_font_free(context->textState.font);
        context->textState.font = font;
        plutovg_canvas_set_font_face(context->canvas, NULL);
        plutovg_font_face_destroy(context->textState.fontface);
    }

    // repair font
    // repair_cmap(context->font);
    // set font face
    // context->fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf",
    // 0); FT_Face face;
    font->load_succeed = false;
    if (font->font_data == NULL)
    {

        // FT_New_Face(context->ft_library, "fonts/SimSun.ttf", 0, &face);
        context->textState.fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf", 0);
        font->load_succeed = true;
    }
    else
    {
        // FT_New_Memory_Face(context->ft_library, font->font_data,
        // font->font_data_length, 0, &face);
        if ((context->textState.fontface = plutovg_font_face_load_from_data(
            font->font_data, font->font_data_length, 0, NULL, NULL)) == NULL)
        {
            font->load_succeed = false;
            context->textState.fontface = plutovg_font_face_load_from_data1(
                font->font_data, font->font_data_length, 0, NULL, NULL);
        }
        else
        {
            font->load_succeed = true;
        }
    }

    plutovg_canvas_set_font(context->canvas, context->textState.fontface, fontsize);
    // set font matrix
    // plutovg_matrix_init_scale(&context->fontMatrixPlutovg, fontsize, fontsize);
    // plutovg_canvas_set_matrix(context->canvas, &context->fontMatrixPlutovg);
}

void handle_Tr(pdf_context_t* context)
{
    // set text rendering mode
    // mode initial value =0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    int v = strtof(buf, NULL);
    // STROKE FILL BOTH CLIP

    int mode = 0;
    if ((v & 0x1) == 0)
    {
        mode |= 2;
    }
    if ((v & 0x4) != 0)
    {
        mode |= 4;
    }
    if (((v & 0x1) ^ ((v & 0x2) >> 1)) != 0)
    {
        mode |= 1;
    }
    context->textState.textMode = mode;
}

void handle_Ts(pdf_context_t* context)
{
    // set text rise
    // rise initial value=0
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float r = strtof(buf, NULL);

    context->textState.textRise = r;
}

void handle_Td(pdf_context_t* context)
{
    // set start position on the page
    // tx ty
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ty = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float tx = strtof(buf, NULL);

    plutovg_canvas_translate(context->canvas, tx, ty);
    plutovg_canvas_move_to(context->canvas, 0, 0);
}

void handle_TD(pdf_context_t* context)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    float ty = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    float tx = strtof(buf, NULL);

    plutovg_canvas_translate(context->canvas, tx, ty);
    plutovg_canvas_move_to(context->canvas, 0, 0);
    context->textState.textLeading = -ty;
    context->textState.lineWidth = 0;
    // side effect, set the leading parameter in the text state
    // -ty TL
    // tx ty Td
}

void handle_Tm(pdf_context_t* context)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    float a, b, c, d, e, f;
    pdf_stack_pop(context->stack, &node);
    f = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    e = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    d = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    c = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    b = strtof(buf, NULL);
    pdf_stack_pop(context->stack, &node);
    a = strtof(buf, NULL);

    plutovg_matrix_t m;
    plutovg_matrix_init(&m, a, b, c, d, e, f);
    plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
    // set font matrix
    plutovg_canvas_set_matrix(context->canvas, &m);
    plutovg_canvas_move_to(context->canvas, 0, 0);
}

void handle_T_star(pdf_context_t* context)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    plutovg_canvas_translate(context->canvas, 0, -context->textState.textLeading);
    plutovg_canvas_move_to(context->canvas, 0, 0);
    context->textState.lineWidth = 0;
}

void handle_Tj(pdf_context_t* context)
{
    // show / paint the glyphs for a string
    // string
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);

    if (buf[0] == '<' && context->textState.font)
    {
        float x, y;
        plutovg_canvas_get_current_point(context->canvas, &x, &y);

        int buf_len = 0;
        int unicode_cnt = 0;
        uint16_t unicode[1024] = { 0 };
        if (strstr(context->textState.font->encoding, "Identity"))
        {
            for (char* p = &buf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                unicode[unicode_cnt++] = t;
            }
        }
        else
        {
            for (char* p = &buf[1]; *p != '\0'; p += 4)
            {
                uint16_t t = _hex_str_to_16bit(p);
                bool found = false;
                pdf_cmap_t* cmap = context->textState.font->cmap;
                while (cmap != NULL && !found)
                {
                    for (int k = 0; k < cmap->unicode_map_len; k++)
                    {
                        if (t == cmap->unicode_map[k].cid)
                        {
                            unicode[unicode_cnt++] = cmap->unicode_map[k].unicode;
                            found = true;
                            break;
                        }
                    }

                    for (int k = 0; k < cmap->char_range_map_len && !found; k++)
                    {
                        if (t >= cmap->char_range_map[k].srcStart &&
                            t <= cmap->char_range_map[k].srcEnd)
                        {
                            unicode[unicode_cnt++] = cmap->char_range_map[k].dstStart +
                                (t - cmap->char_range_map[k].srcStart);
                            found = true;
                            break;
                        }
                    }
                    cmap = cmap->next;
                }
                if (!found)
                {
                    unicode[unicode_cnt++] = t;
                }
            }
        }

        plutovg_canvas_save(context->canvas);
        // cos(theta), sin(theta),  0
        // -sin(theta), cos(theta), 0
        // 0, 0, 1

        // 1,  0, 0
        // 0, -1, 0,
        // 0,  0, 1
        // rotate 180æŽ?
        // or scale by 1
        plutovg_canvas_scale(context->canvas, 1, -1);
        plutovg_canvas_set_font_size(context->canvas, context->textState.fontSize);
        plutovg_canvas_set_font_face(context->canvas, context->textState.fontface);
        plutovg_canvas_set_rgb(context->canvas,
            context->graphicsState.fillColor[0],
            context->graphicsState.fillColor[1],
            context->graphicsState.fillColor[2]);
        printf("before %.2f\n", context->textState.lineWidth);
        if (context->textState.font->load_succeed)
        {
            if (strstr(context->textState.font->encoding, "Identity"))
                context->textState.lineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->textState.lineWidth, 0);
            else
                context->textState.lineWidth += plutovg_canvas_fill_text(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, context->textState.lineWidth, 0);
        }
        else
            context->textState.lineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                PLUTOVG_TEXT_ENCODING_UTF16, context->textState.lineWidth, 0);
        plutovg_canvas_restore(context->canvas);
        printf("after %.2f\n", context->textState.lineWidth);
    }
}

void handle_apostrophe(pdf_context_t* context)
{
    // move to the next line and show a text string
    // string
    // same as
    // T*
    // string Tj
    context->textState.lineWidth = 0;
    handle_T_star(context);
    handle_Tj(context);
}

void handle_quotation(pdf_context_t* context)
{
    // move to the next line and show a text string
    // aw as the word spacing
    // ac as the character spacing
    // aw ac string
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    context->textState.lineWidth = 0;
    // TODO
}

void handle_TJ(pdf_context_t* context)
{
    // show one or more text strings
    // array
    // if the element is a string , show the string
    // if the element is a number, adjust the position
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_t* tmp_stack = pdf_stack_init();
    while (true)
    {
        pdf_stack_pop(context->stack, &node);
        if (!strcmp(buf, "]"))
        {
            // ignore
        }
        else if (!strcmp(buf, "["))
        {
            break;
        }
        else
        {
            pdf_stack_push(tmp_stack, buf, strlen(buf));
        }
    }
    memset(buf, 0, sizeof(buf));
    while (tmp_stack->top != NULL)
    {
        pdf_stack_pop(tmp_stack, &node);
        if (buf[0] == '<' && context->textState.font)
        {
            float x, y;
            plutovg_canvas_get_current_point(context->canvas, &x, &y);

            int buf_len = 0;
            int unicode_cnt = 0;
            uint16_t unicode[1024] = { 0 };
            if (strstr(context->textState.font->encoding, "Identity"))
            {
                for (char* p = &buf[1]; *p != '\0'; p += 4)
                {
                    uint16_t t = _hex_str_to_16bit(p);
                    unicode[unicode_cnt++] = t;
                }
            }
            else
            {
                for (char* p = &buf[1]; *p != '\0'; p += 4)
                {
                    uint16_t t = _hex_str_to_16bit(p);
                    bool found = false;
                    pdf_cmap_t* cmap = context->textState.font->cmap;
                    while (cmap != NULL && !found)
                    {
                        for (int k = 0; k < cmap->unicode_map_len; k++)
                        {
                            if (t == cmap->unicode_map[k].cid)
                            {
                                unicode[unicode_cnt++] = cmap->unicode_map[k].unicode;
                                found = true;
                                break;
                            }
                        }

                        for (int k = 0; k < cmap->char_range_map_len && !found; k++)
                        {
                            if (t >= cmap->char_range_map[k].srcStart &&
                                t <= cmap->char_range_map[k].srcEnd)
                            {
                                unicode[unicode_cnt++] = cmap->char_range_map[k].dstStart +
                                    (t - cmap->char_range_map[k].srcStart);
                                found = true;
                                break;
                            }
                        }
                        cmap = cmap->next;
                    }
                    if (!found)
                    {
                        unicode[unicode_cnt++] = t;
                    }
                }
            }

            plutovg_canvas_save(context->canvas);
            // cos(theta), sin(theta),  0
            // -sin(theta), cos(theta), 0
            // 0, 0, 1

            // 1,  0, 0
            // 0, -1, 0,
            // 0,  0, 1
            // rotate 180æŽ?
            // or scale by 1
            plutovg_canvas_scale(context->canvas, 1, -1);
            plutovg_canvas_set_font_size(context->canvas, context->textState.fontSize);
            plutovg_canvas_set_font_face(context->canvas, context->textState.fontface);
            plutovg_canvas_set_rgb(context->canvas,
                context->graphicsState.fillColor[0],
                context->graphicsState.fillColor[1],
                context->graphicsState.fillColor[2]);
            if (context->textState.font->load_succeed)
            {
                if (strstr(context->textState.font->encoding, "Identity"))
                    context->textState.lineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                        PLUTOVG_TEXT_ENCODING_UTF16, x + context->textState.lineWidth, y);
                else
                    context->textState.lineWidth += plutovg_canvas_fill_text(context->canvas, unicode, unicode_cnt,
                        PLUTOVG_TEXT_ENCODING_UTF16, x + context->textState.lineWidth, y);
            }
            else
                context->textState.lineWidth += plutovg_canvas_fill_text1(context->canvas, unicode, unicode_cnt,
                    PLUTOVG_TEXT_ENCODING_UTF16, x + context->textState.lineWidth, y);

            plutovg_canvas_restore(context->canvas);
        }
        else if (buf[0] == '(')
        {

        }
        else // a number
        {
            float a = strtof(buf, NULL);
            //plutovg_canvas_translate(context->canvas, -a, 0);
            //plutovg_canvas_move_to(context->canvas, 0, 0);
            context->textState.lineWidth -= (a * (context->textState.fontSize / 1000.0));
        }
    }
    pdf_stack_free(tmp_stack);
}

void handle_d0(pdf_context_t* context)
{
    // wx wy
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_d1(pdf_context_t* context)
{
    // wx wy llx lly urx ury
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}

void handle_BDC(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}
void handle_BMC(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}
void handle_DP(pdf_context_t* context)
{
    // tag properties
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
    pdf_stack_pop(context->stack, &node);
}
void handle_EMC(pdf_context_t* context) {}
void handle_MP(pdf_context_t* context)
{
    // tag
    char buf[1024] = { 0 };
    pdf_stack_node_t node;
    node.data = buf;
    pdf_stack_pop(context->stack, &node);
}