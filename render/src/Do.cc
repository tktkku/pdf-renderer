#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
#include <algorithm>
void handle_Do(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // paint a specified XObject
    // name
    if (dry_run)
    {
        auto data = context->deque->pop_front();

        //pdf_obj_t* tmp_obj = NULL;
        pdf_xobject_t* xobj = NULL;
        if (context->current_obj != NULL)
        {
            // pdf_dict* tmp_dict = pdf_dict_get_dict(context->current_obj->value->val.dict, "/Resources");
            // tmp_dict = pdf_dict_get_dict(tmp_dict, "/XObject");
            // int ref = pdf_dict_get_ref(tmp_dict, buf);
            // tmp_obj = pdf_file_get_obj(context->page->pdf, ref);
            xobj = pdf_obj_get_xobject(context->current_obj, (char*)data->data());
        }
        else
        {
            return;
        }
        // if (xobj == NULL)
        // {
            
        // }
        //xobj = pdf_obj_get_xobject(context->page->obj, buf);
        if (xobj == NULL)
            return;
        if (xobj->type == XOBJ_FORM)
        {
            if (xobj->obj->stream != NULL)
            {
                pdf_obj_t* save_obj = context->current_obj;
                context->current_obj = xobj->obj;
                pdf_stream_open(xobj->obj->stream);
                pdf_token* tk = NULL;
                cmd->type = TOKEN_OPERATOR_Do;
                cmd->Do.type = XOBJ_FORM;
                cmd->Do.xobj = xobj;
                while ((tk = pdf_parser_next_token(xobj->obj->stream->parser)) != NULL)
                {
                    if (tk->type() == TOKEN_STREAM_END)
                        break;
                    pdf_render_command* new_cmd = _do_render_operation(xobj->obj->stream, context, tk);
                    if (new_cmd)
                    {
                        std::unique_ptr<pdf_render_command> u(new_cmd);
                        cmd->Do.opts.push_back(std::move(u));
                    }
                    
                    delete tk;
                }

                pdf_stream_close(xobj->obj->stream);
                context->current_obj = save_obj;
            }
        }
        else if (xobj->type == XOBJ_IMAGE)
        {
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

            cmd->type = TOKEN_OPERATOR_Do;
            cmd->Do.type = XOBJ_IMAGE;
            cmd->Do.width = xobj->image->width;
            cmd->Do.height = xobj->image->height;
            cmd->Do.channels = channels;
            cmd->Do.pixels = (unsigned char*)xobj->image->data;
            cmd->Do.pixels_size = xobj->image->data_len;
        }
    }
    else
    {
        if (cmd->Do.type == XOBJ_IMAGE)
        {
            PDF_RENDERER_CALL(context->renderer, draw_image, 
                cmd->Do.width, cmd->Do.height, 
                cmd->Do.channels, cmd->Do.pixels, cmd->Do.pixels_size);
        }
        else
        {
            PDF_RENDERER_CALL(context->renderer, save);
            PDF_RENDERER_CALL(context->renderer, transform, 
                cmd->Do.xobj->form->matrix[0], cmd->Do.xobj->form->matrix[1],
                cmd->Do.xobj->form->matrix[2], cmd->Do.xobj->form->matrix[3],
                cmd->Do.xobj->form->matrix[4], cmd->Do.xobj->form->matrix[5]);
            for (auto& command : cmd->Do.opts)
            {
                handlers[command->type - TOKEN_OPERATOR](context, command.get(), false);
            }
            PDF_RENDERER_CALL(context->renderer, restore);
        }
    }
    
}
