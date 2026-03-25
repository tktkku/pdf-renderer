#include "pdf-render.h"
#include "pdf-render-private.h"
std::function<void()> handle_apostrophe(pdf_render* context)
{
    // move to the next line and show a text string
    // string
    // same as
    // T*
    // string Tj
    plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
    context->state->textState.textLineWidth = 0;
    auto data = context->deque->pop_front();
    int len = data->size();
    char* raw = new char[len];
    std::shared_ptr<char> text(raw, std::default_delete<char[]>());
    memcpy(text.get(), data->data(), len);
    return [context, text, len] {
        if (context->state->textState.font == NULL)
            return;
        _do_text_render(context, (char*)text.get(), len);
    };
}

std::function<void()> handle_BT(pdf_render* context)
{
    return [context] {
        plutovg_canvas_save(context->canvas);
        //plutovg_canvas_move_to(context->canvas, 0, 0);
        // context->fontface = NULL;
        // context->font = NULL;
        context->state->textState.textLineWidth = 0;
        plutovg_matrix_init_identity(&context->state->textState.textMatrix);
    }; 
    // begin text
}

std::function<void()> handle_ET(pdf_render* context)
{
    // end text
   return [context] {
        plutovg_canvas_restore(context->canvas);
   };
}

std::function<void()> handle_quotation(pdf_render* context)
{
    // move to the next line and show a text string
    // aw as the word spacing
    // ac as the character spacing
    // aw ac string
    context->deque->pop_front();
    context->deque->pop_front();
    context->deque->pop_front();
    return [context] {
        context->state->textState.textLineWidth = 0;
    };
}

std::function<void()> handle_T_star(pdf_render* context)
{
    // move to the start of the next line
    // has the same effects as the code
    // 0 -[current leading matrix] Td
    // float x, y;
    // plutovg_canvas_get_current_point(context->canvas, &x, &y);
    // plutovg_canvas_move_to(context->canvas, x, y);
    // plutovg_canvas_translate(context->canvas, 0, -context->state->textState.textLeading);
    // plutovg_canvas_move_to(context->canvas, 0, 0);
    return [context] {
        plutovg_matrix_translate(&context->state->textState.textMatrix, 0, -context->state->textState.textLeading);
        context->state->textState.textLineWidth = 0;
    };
}

std::function<void()> handle_Tc(pdf_render* context)
{
    // character spacing
    // used by Tj TJ '
    // charSpace initial value=0
    auto data = context->deque->pop_front();
    float c = strtof(data->data(), NULL);
    return [context, c] {
        context->state->textState.characterSpacing = c;
    };
}

std::function<void()> handle_Td(pdf_render* context)
{
    // set start position on the page
    // tx ty
    auto data = context->deque->pop_front();
    float ty = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float tx = strtof(data2->data(), NULL);

    return [context, tx, ty] {
        plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
        context->state->textState.textLineWidth = 0;
    };
    // plutovg_canvas_translate(context->canvas, tx, ty);
    // plutovg_canvas_move_to(context->canvas, 0, 0);

}

std::function<void()> handle_TD(pdf_render* context)
{
    // move to the start of the next line
    // offset form the start of the current line
    // tx ty
    auto data = context->deque->pop_front();
    float ty = strtof(data->data(), NULL);
    auto data2 = context->deque->pop_front();
    float tx = strtof(data2->data(), NULL);
    return [context, tx, ty] {
        // plutovg_canvas_translate(context->canvas, tx, ty);
        // plutovg_canvas_move_to(context->canvas, 0, 0);
        plutovg_matrix_translate(&context->state->textState.textMatrix, tx, ty);
        context->state->textState.textLeading = -ty;
        context->state->textState.textLineWidth = 0;
        // side effect, set the leading parameter in the text state
        // -ty TL
        // tx ty Td
    };
}

std::function<void()> handle_Tj(pdf_render* context)
{
    // show / paint the glyphs for a string
    // string
    auto data = context->deque->pop_front();
    int len = data->size();
    char* raw = new char[len];
    std::shared_ptr<char> text(raw, std::default_delete<char[]>());
    memcpy(text.get(), data->data(), len);
    return [context, text, len] {
        if (context->state->textState.font == NULL)
            return;
        _do_text_render(context, (char*)text.get(), len);
    };
}

std::function<void()> handle_TJ(pdf_render* context)
{
    // show one or more text strings
    // array
    // if the element is a string , show the string
    // if the element is a number, adjust the position
    auto tmp_deque = std::make_shared<pdf_deque>();
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
    return [context, tmp_deque] {
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
    };
}

std::function<void()> handle_TL(pdf_render* context)
{
    // text leading
    // used by T* ' "
    // leading initial value = 0
    auto data = context->deque->pop_front();
    float t = strtof(data->data(), NULL);
    return [context, t] {
        context->state->textState.textLeading = t;
    };
}

std::function<void()> handle_Tm(pdf_render* context)
{
    // set the text matrix, and the text line matrix
    // a b c d e f
    
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
    return [context, a, b, c, e, d, f] {
        plutovg_matrix_t m;
        plutovg_matrix_init(&m, a, b, c, d, e, f);

        //plutovg_matrix_multiply(&m, &context->textState.fontMatrixPlutovg, &m);
        // set font matrix
        // plutovg_canvas_transform(context->canvas, &m);
        // plutovg_canvas_move_to(context->canvas, 0, 0);
        context->state->textState.textMatrix = m;
        context->state->textState.textLineWidth = 0;
    };
}

std::function<void()> handle_Tr(pdf_render* context)
{
    // set text rendering mode
    // mode initial value =0
    auto data = context->deque->pop_front();
    int v = strtof(data->data(), NULL);
    // STROKE FILL BOTH CLIP
    return [context, v] {
        context->state->textState.textMode = v;
    };
}

std::function<void()> handle_Ts(pdf_render* context)
{
    // set text rise
    // rise initial value=0
    auto data = context->deque->pop_front();   
    float r = strtof(data->data(), NULL);
    return [context, r] {
        context->state->textState.textRise = r;
    };
}

std::function<void()> handle_Tw(pdf_render* context)
{
    // word spacing
    // used by Tj TJ '
    // wordSpace initial value=0
    auto data = context->deque->pop_front();
    float w = strtof(data->data(), NULL);
    return [context, w] {
        context->state->textState.wordSpacing = w;
    };
}

std::function<void()> handle_Tz(pdf_render* context)
{
    // horizontal scaling
    // scale initial value=100
    auto data = context->deque->pop_front();
    float h = strtof(data->data(), NULL);
    return [context, h] {
        context->state->textState.horizontalScaling = h;
    };
    //plutovg_canvas_scale(context->canvas, h / 100.0, 1.0);
}
