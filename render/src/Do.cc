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
            cmd->type = TOKEN_OPERATOR_Do;
            cmd->Do.type = XOBJ_FORM;
            cmd->Do.xobj = xobj;
            if (xobj->obj->stream != NULL)
            {
                pdf_obj_t* save_obj = context->current_obj;
                context->current_obj = xobj->obj;
                // ponytail: cache decompressed stream data so repeated Do only re-parses
                if (xobj->form->cached_stream_data == NULL)
                {
                    pdf_stream_open(xobj->obj->stream);
                    pdf_stream_get_all(xobj->obj->stream,
                        &xobj->form->cached_stream_data, &xobj->form->cached_stream_len);
                    pdf_stream_close(xobj->obj->stream);
                }
                // re-parse from cached data (fast, no decompression needed)
                input_t* cached_input = NULL;
                input_buffer(&cached_input, (char*)xobj->form->cached_stream_data, xobj->form->cached_stream_len);
                pdf_parser_t* cached_parser = pdf_parser_init(context->pdf, cached_input);
                xobj->obj->stream->parser = cached_parser;
                pdf_token* tk = NULL;
                while ((tk = pdf_parser_next_token(cached_parser)) != NULL)
                {
                    if (tk->type() == TOKEN_STREAM_END) { delete tk; break; }
                    pdf_render_command* new_cmd = _do_render_operation(xobj->obj->stream, context, tk);
                    if (new_cmd)
                        cmd->Do.opts.push_back(std::unique_ptr<pdf_render_command>(new_cmd));
                    delete tk;
                }
                xobj->obj->stream->parser = NULL;
                pdf_parser_free(cached_parser);
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
