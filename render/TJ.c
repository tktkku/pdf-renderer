#include "render.h"
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
    if (context->state->textState.font == NULL)
    {
        pdf_stack_free(tmp_stack);
        return;
    }
    memset(buf, 0, sizeof(buf));
    while (tmp_stack->top != NULL)
    {
        pdf_stack_pop(tmp_stack, &node);
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

    pdf_stack_free(tmp_stack);
}
