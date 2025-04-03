#include "render.h"
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
