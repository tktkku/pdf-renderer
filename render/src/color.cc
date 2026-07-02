#include "pdf-render.h"
#include "pdf-render-private.h"
// ponytail: pop helpers to eliminate ~150 lines of duplicate pop+strtof boilerplate
static inline float pop_float(pdf_deque* deque) {
    auto data = deque->pop_front();
    return strtof(data->data(), NULL);
}
static inline void pop_gray_cmd(pdf_render_command* cmd, pdf_deque* deque, pdf_token_type_t type) {
    float g = pop_float(deque);
    cmd->type = type;
    cmd->color.g = g; cmd->color.r = g; cmd->color.b = g;
}
static inline void pop_rgb_cmd(pdf_render_command* cmd, pdf_deque* deque, pdf_token_type_t type) {
    float b = pop_float(deque), g = pop_float(deque), r = pop_float(deque);
    cmd->type = type;
    cmd->color.r = r; cmd->color.g = g; cmd->color.b = b;
}
static inline void pop_cmyk_cmd(pdf_render_command* cmd, pdf_deque* deque, pdf_token_type_t type) {
    float k = pop_float(deque), y = pop_float(deque), m = pop_float(deque), c = pop_float(deque);
    cmd->type = type;
    cmd->color.r = (1.f - c) * (1.f - k);
    cmd->color.g = (1.f - m) * (1.f - k);
    cmd->color.b = (1.f - y) * (1.f - k);
}
void _get_color_space(pdf_render* context, char* name, char* value)
{
    if (context->current_obj->resources.colorspace_dict 
        && context->current_obj->resources.colorspace_dict->has(name))
    {
        if (context->current_obj->resources.colorspace_dict->is_name(name))
        {
            char* color_space = context->current_obj->resources.colorspace_dict->get_name(name);
            if (color_space != NULL)
            {
                strcpy(value, color_space);
                return;
            }
        }
        else if (context->current_obj->resources.colorspace_dict->is_indirect(name))
        {
            pdf_indirect_t ref = context->current_obj->resources.colorspace_dict->get_indirect(name);
            pdf_obj_t* color_space_obj = pdf_file_get_obj(context->current_obj->pdf, ref);
            if (color_space_obj != NULL)
            {
                if (color_space_obj->value->type == PDF_VALUE_NAME)
                {
                    strcpy(value, color_space_obj->value->val.name);
                    return;
                }
            }
        }
    }
}
void handle_cs(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_cs;
        memcpy(cmd->colorSpace, data->data(), data->size());
    } else {
        if (!strcmp(cmd->colorSpace, "/DeviceGray") || !strcmp(cmd->colorSpace, "/DeviceRGB") || !strcmp(cmd->colorSpace, "/DeviceCMYK"))
            strcpy(context->state->fill.currentColorSpace, cmd->colorSpace);
        else
            _get_color_space(context, cmd->colorSpace, context->state->fill.currentColorSpace);
    }
}

void handle_CS(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        auto data = context->deque->pop_front();
        cmd->type = TOKEN_OPERATOR_CS;
        memcpy(cmd->colorSpace, data->data(), data->size());
    } else {
        if (!strcmp(cmd->colorSpace, "/DeviceGray") || !strcmp(cmd->colorSpace, "/DeviceRGB") || !strcmp(cmd->colorSpace, "/DeviceCMYK"))
            strcpy(context->state->stroke.currentColorSpace, cmd->colorSpace);
        else
            _get_color_space(context, cmd->colorSpace, context->state->stroke.currentColorSpace);
    }
}

void handle_g(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_gray_cmd(cmd, context->deque, TOKEN_OPERATOR_g);
    } else {
        context->state->fill.color[0] = cmd->color.g;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.g;
        strcpy(context->state->fill.currentColorSpace, "/DeviceGray");
    }
}

void handle_G(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_gray_cmd(cmd, context->deque, TOKEN_OPERATOR_G);
    } else {
        context->state->stroke.color[0] = cmd->color.g;
        context->state->stroke.color[1] = cmd->color.g;
        context->state->stroke.color[2] = cmd->color.g;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceGray");
    }
}

void handle_k(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_cmyk_cmd(cmd, context->deque, TOKEN_OPERATOR_k);
    } else {
        context->state->fill.color[0] = cmd->color.r;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.b;
        strcpy(context->state->fill.currentColorSpace, "/DeviceCMYK");
    }
}

void handle_K(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_cmyk_cmd(cmd, context->deque, TOKEN_OPERATOR_K);
    } else {
        context->state->stroke.color[0] = cmd->color.r;
        context->state->stroke.color[1] = cmd->color.g;
        context->state->stroke.color[2] = cmd->color.b;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceCMYK");
    }
}

void handle_rg(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_rg);
    } else {
        context->state->fill.color[0] = cmd->color.r;
        context->state->fill.color[1] = cmd->color.g;
        context->state->fill.color[2] = cmd->color.b;
        strcpy(context->state->fill.currentColorSpace, "/DeviceRGB");
    }
}

void handle_RG(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (dry_run) {
        pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_RG);
    } else {
        context->state->stroke.color[0] = cmd->color.r;
        context->state->stroke.color[1] = cmd->color.g;
        context->state->stroke.color[2] = cmd->color.b;
        strcpy(context->state->stroke.currentColorSpace, "/DeviceRGB");
    }
}

void handle_sc(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray"))
    {
        if (dry_run) { pop_gray_cmd(cmd, context->deque, TOKEN_OPERATOR_sc); }
        else {
            context->state->fill.color[0] = cmd->color.g;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.g;
        }
    }
    else if (!strcmp(context->state->fill.currentColorSpace, "/DeviceRGB"))
    {
        if (dry_run) { pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_sc); }
        else {
            context->state->fill.color[0] = cmd->color.r;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.b;
        }
    }
    else if (!strcmp(context->state->fill.currentColorSpace, "/DeviceCMYK"))
    {
        if (dry_run) { pop_cmyk_cmd(cmd, context->deque, TOKEN_OPERATOR_sc); }
        else {
            context->state->fill.color[0] = cmd->color.r;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.b;
        }
    }
    else
    {
        if (dry_run) { pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_sc); }
        else {
            context->state->fill.color[0] = cmd->color.r;
            context->state->fill.color[1] = cmd->color.g;
            context->state->fill.color[2] = cmd->color.b;
        }
    }
}

void handle_SC(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray"))
    {
        if (dry_run) { pop_gray_cmd(cmd, context->deque, TOKEN_OPERATOR_SC); }
        else {
            context->state->stroke.color[0] = cmd->color.g;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.g;
        }
    }
    else if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceRGB"))
    {
        if (dry_run) { pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_SC); }
        else {
            context->state->stroke.color[0] = cmd->color.r;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.b;
        }
    }
    else if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceCMYK"))
    {
        if (dry_run) { pop_cmyk_cmd(cmd, context->deque, TOKEN_OPERATOR_SC); }
        else {
            context->state->stroke.color[0] = cmd->color.r;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.b;
        }
    }
    else
    {
        if (dry_run) { pop_rgb_cmd(cmd, context->deque, TOKEN_OPERATOR_SC); }
        else {
            context->state->stroke.color[0] = cmd->color.r;
            context->state->stroke.color[1] = cmd->color.g;
            context->state->stroke.color[2] = cmd->color.b;
        }
    }
}

void handle_scn(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (!strcmp(context->state->fill.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->fill.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_sc(context, cmd, dry_run);
    }
    // For pattern color spaces, set to white
    if (dry_run) {
        cmd->type = TOKEN_OPERATOR_scn;
        cmd->color.r = 1.0f; cmd->color.g = 1.0f; cmd->color.b = 1.0f;
    } else {
        context->state->fill.color[0] = 1.0;
        context->state->fill.color[1] = 1.0;
        context->state->fill.color[2] = 1.0;
    }
}

void handle_SCN(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    if (!strcmp(context->state->stroke.currentColorSpace, "/DeviceGray") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceRGB") 
    || !strcmp(context->state->stroke.currentColorSpace, "/DeviceCMYK"))
    {
        return handle_SC(context, cmd, dry_run);
    }
    // For pattern color spaces, set to white
    if (dry_run) {
        cmd->type = TOKEN_OPERATOR_SCN;
        cmd->color.r = 1.0f; cmd->color.g = 1.0f; cmd->color.b = 1.0f;
    } else {
        context->state->stroke.color[0] = 1.0;
        context->state->stroke.color[1] = 1.0;
        context->state->stroke.color[2] = 1.0;
    }
}