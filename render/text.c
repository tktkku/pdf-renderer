#include "render.h"
#include "pdf-private.h"
#include <plutovg-private.h>
void handle_apostrophe(pdf_context_t* context)
{
    // move to the next line and show a text string
    // string
    // same as
    // T*
    // string Tj
    context->state->textState.textLineWidth = 0;
    handle_T_star(context);
    handle_Tj(context);
}

void handle_BT(pdf_context_t* context)
{
    // begin text
    plutovg_canvas_save(context->canvas);
    //plutovg_canvas_move_to(context->canvas, 0, 0);
    // context->fontface = NULL;
    // context->font = NULL;
    context->state->textState.textLineWidth = 0;
    plutovg_matrix_init_identity(&context->state->textState.textMatrix);
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

void handle_quotation(pdf_context_t* context)
{
    // move to the next line and show a text string
    // aw as the word spacing
    // ac as the character spacing
    // aw ac string
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
    pdf_deque_pop_front(context->deque, &node);
    context->state->textState.textLineWidth = 0;
    // TODO
}

void handle_T_star(pdf_context_t* context)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    // plutovg_canvas_translate(context->canvas, 0, -context->state->textState.textLeading);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
    context->state->textState.textLineWidth = 0;
}

void handle_Tc(pdf_context_t* context)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float c = strtof(buf, NULL);
    context->state->textState.characterSpacing = c;
}

void handle_Td(pdf_context_t* context)
{
    // set start position on the page
    // tx ty
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float ty = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float tx = strtof(buf, NULL);

    // plutovg_canvas_translate(context->canvas, tx, ty);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
    context->state->textState.textLineWidth = 0;
}

void handle_TD(pdf_context_t* context)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float ty = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    float tx = strtof(buf, NULL);

    // plutovg_canvas_translate(context->canvas, tx, ty);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
    context->state->textState.textLeading = -ty;
    context->state->textState.textLineWidth = 0;
    // side effect, set the leading parameter in the text state
    // -ty TL
    // tx ty Td
}

void handle_Tj(pdf_context_t* context)
{
    // show / paint the glyphs for a string
    // string
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    if (context->state->textState.font == NULL)
        return;

    _do_text_render(context, node.data, node.size);
}

void handle_TJ(pdf_context_t* context)
{
    // show one or more text strings
    // array
    // if the element is a string , show the string
    // if the element is a number, adjust the position
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_t* tmp_deque = pdf_deque_init();
    while (true)
    {
        pdf_deque_pop_front(context->deque, &node);
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
            pdf_deque_push(tmp_deque, buf, strlen(buf));
        }
    }
    if (context->state->textState.font == NULL)
    {
        pdf_deque_free(tmp_deque);
        return;
    }
    memset(buf, 0, sizeof(buf));
    while (tmp_deque->size > 0)
    {
        pdf_deque_pop_front(tmp_deque, &node);
        if (buf[0] == '<' || buf[0] == '(')
        {
            _do_text_render(context, node.data, node.size);
        }
        else // a number
        {
            float a = strtof(buf, NULL);
            //plutovg_canvas_translate(context->canvas, -a, 0);
            //plutovg_canvas_move_to(context->canvas, 0, 0);
            context->state->textState.textLineWidth -= (a * (context->state->textState.fontSize / 1000.0));
        }
    }

    pdf_deque_free(tmp_deque);
}

void handle_TL(pdf_context_t* context)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float t = strtof(buf, NULL);
    context->state->textState.textLeading = t;
}

void handle_Tm(pdf_context_t* context)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    float a, b, c, d, e, f;
    pdf_deque_pop_front(context->deque, &node);
    f = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    e = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    d = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    c = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    b = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    a = strtof(buf, NULL);

    plutovg_matrix_t m;
    plutovg_matrix_init(&m, a, b, c, d, e, f);

    //plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
    // set font matrix
    // plutovg_canvas_transform(context->canvas, &m);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    context->state->textState.textMatrix = m;
    context->state->textState.textLineWidth = 0;
}

void handle_Tr(pdf_context_t* context)
{
    // set text rendering mode
    // mode initial value =0
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    int v = strtof(buf, NULL);
    // STROKE FILL BOTH CLIP

    context->state->textState.textMode = v;
}

void handle_Ts(pdf_context_t* context)
{
    // set text rise
    // rise initial value=0
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float r = strtof(buf, NULL);

    context->state->textState.textRise = r;
}

void handle_Tw(pdf_context_t* context)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float w = strtof(buf, NULL);
    context->state->textState.wordSpacing = w;
}

void handle_Tz(pdf_context_t* context)
{
    // horizontal scaling
    // scale initial value=100
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float h = strtof(buf, NULL);
    //plutovg_canvas_scale(context->canvas, h / 100.0, 1.0);
    context->state->textState.horizontalScaling = h;
}
