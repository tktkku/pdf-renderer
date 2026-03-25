#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
void handle_apostrophe(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // move to the next line and show a text string
    // string
    // same as
    // T*
    // string Tj
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int len = data->size();
        cmd->type = TOKEN_OPERATOR_apostrophe;
        new (&cmd->apostrophe.data) std::unique_ptr<pdf_node>(std::move(data));
    }
    else
    {
        plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
        context->state->textState.textLineWidth = 0;
        if (context->state->textState.font == NULL)
            return;
        _do_text_render(context, (char*)cmd->apostrophe.data->data(), cmd->apostrophe.data->size());
    }
}

void handle_BT(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_BT;
    }
    else
    {
        plutovg_canvas_save(context->canvas);
        //plutovg_canvas_move_to(context->canvas, 0, 0);
        // context->fontface = NULL;
        // context->font = NULL;
        context->state->textState.textLineWidth = 0;
        plutovg_matrix_init_identity(&context->state->textState.textMatrix);
    }
}

void handle_ET(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // end text
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_ET;
    }
    else
    {
        plutovg_canvas_restore(context->canvas);
    };
}

void handle_quotation(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // move to the next line and show a text string
    // aw as the word spacing
    // ac as the character spacing
    // aw ac string
    if (dry_run)
    {
        context->deque->pop_front();
        context->deque->pop_front();
        context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_quotation;
    }
    else
    {
        context->state->textState.textLineWidth = 0;
    }
}

void handle_T_star(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    // plutovg_canvas_translate(context->canvas, 0, -context->state->textState.textLeading);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    if (dry_run)
    {
        cmd->type = TOKEN_OPERATOR_T_star;
    }
    else
    {
        plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
        context->state->textState.textLineWidth = 0;
    };
}

void handle_Tc(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float c = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Tc;
        cmd->Tc.f = c;
    }
    else
    {
        context->state->textState.characterSpacing = cmd->Tc.f;
    }
}

void handle_Td(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set start position on the page
    // tx ty
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float ty = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float tx = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Td;
        cmd->Td.x = tx;
        cmd->Td.y = ty;
    }
    else
    {
        plutovg_matrix_translate(&context->state->textState.textMatrix, cmd->Td.x, cmd->Td.y);
        context->state->textState.textLineWidth = 0;
    }
}

void handle_TD(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float ty = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        float tx = strtof(data2->data(), NULL);
        cmd->type = TOKEN_OPERATOR_TD;
        cmd->TD.x = tx;
        cmd->TD.y = ty;
    }
    else
    {
        plutovg_matrix_translate(&context->state->textState.textMatrix, cmd->TD.x, cmd->TD.y);
        context->state->textState.textLeading = -cmd->TD.y;
        context->state->textState.textLineWidth = 0;
    }
}

void handle_Tj(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // show / paint the glyphs for a string
    // string
    if (dry_run)
    {
        std::unique_ptr<pdf_node> data = context->deque->pop_front();
        int len = data->size();
        cmd->type = TOKEN_OPERATOR_Tj;
        new (&cmd->Tj.data) std::unique_ptr<pdf_node>(std::move(data));
    }
    else
    {
        if (context->state->textState.font == NULL)
            return;
        _do_text_render(context, (char*)cmd->Tj.data->data(), cmd->Tj.data->size());
    }
}

void handle_TJ(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // show one or more text strings
    // array
    // if the element is a string , show the string
    // if the element is a number, adjust the position
    if (dry_run)
    {
        auto tmp_deque = std::shared_ptr<pdf_deque>();
        while (true)
        {
            auto data = context->deque->pop_front();
            if (!strcmp(data->data(), "]"))
            {
                // ignore
            }
            else if (!strcmp(data->data(), "["))
            {
                break;
            }
            else
            {
                tmp_deque->push_front(data->data(), data->size());
            }
        }
        cmd->type = TOKEN_OPERATOR_TJ;
        new (&cmd->TJ.tmp_deque) std::shared_ptr<pdf_deque>(std::move(tmp_deque));
    }
    else
    {
        auto tmp_deque = cmd->TJ.tmp_deque;
        if (context->state->textState.font == NULL)
        {
            return;
        }
        int size = tmp_deque->size();
        for (int i = 0; i < size; i++)
        {
            auto data = tmp_deque->get(i);
            if ((data)[0] == '<' || (data)[0] == '(')
            {
                _do_text_render(context, (char*)data.data(), data.size());
            }
            else // a number
            {
                float a = strtof(data.data(), NULL);
                //plutovg_canvas_translate(context->canvas, -a, 0);
                //plutovg_canvas_move_to(context->canvas, 0, 0);
                context->state->textState.textLineWidth -= (a * (context->state->textState.fontSize / 1000.0));
            }
        }
    }
}

void handle_TL(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float t = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_TL;
        cmd->TL.f = t;
    }
    else
    {
        context->state->textState.textLeading = cmd->TL.f;
    }
}

void handle_Tm(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    if (dry_run)
    {
        float a, b, c, d, e, f;
        auto data = context->deque->pop_front();
        f = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        e = strtof(data2->data(), NULL);
        auto data3 = context->deque->pop_front();
        d = strtof(data3->data(), NULL);
        auto data4 = context->deque->pop_front();
        c = strtof(data4->data(), NULL);
        auto data5 = context->deque->pop_front();
        b = strtof(data5->data(), NULL);
        auto data6 = context->deque->pop_front();
        a = strtof(data6->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Tm;
        cmd->Tm.x1 = a;
        cmd->Tm.y1 = b;
        cmd->Tm.x2 = c;
        cmd->Tm.y2 = d;
        cmd->Tm.x3 = e;
        cmd->Tm.y3 = f;
    }
    else
    {
        plutovg_matrix_t m;
        plutovg_matrix_init(&m, cmd->Tm.x1, cmd->Tm.y1, cmd->Tm.x2, cmd->Tm.y2, cmd->Tm.x3, cmd->Tm.y3);

        //plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
        // set font matrix
        // plutovg_canvas_transform(context->canvas, &m);
        // plutovg_canvas_move_to(context->canvas, 0, 0);
        context->state->textState.textMatrix = m;
        context->state->textState.textLineWidth = 0;
    }
}

void handle_Tr(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set text rendering mode
    // mode initial value =0
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        int v = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Tr;
        cmd->Tr.i = v;
    }
    else
    {
        context->state->textState.textMode = cmd->Tr.i;
    }
}

void handle_Ts(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set text rise
    // rise initial value=0
    if (dry_run)
    {
        auto data = context->deque->pop_front();   
        float r = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Ts;
        cmd->Ts.f = r;
    }
    else
    {
        context->state->textState.textRise = cmd->Ts.f;
    }
}

void handle_Tw(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float w = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Tw;
        cmd->Tw.f = w;
    }
    else
    {
        context->state->textState.wordSpacing = cmd->Tw.f;
    }
}

void handle_Tz(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // horizontal scaling
    // scale initial value=100
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float h = strtof(data->data(), NULL);
        cmd->type = TOKEN_OPERATOR_Tz;
        cmd->Tz.f = h;
    }
    else
    {
        context->state->textState.horizontalScaling = cmd->Tz.f;
    }
}
