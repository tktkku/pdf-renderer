#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
#include "plutovg-stb-image-write.h"
#include "plutovg-stb-image.h"
void handle_Do(pdf_render_t* context)
{
    // paint a specified XObject
    // name
    unsigned char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);

    //pdf_obj_t* tmp_obj = NULL;
    pdf_xobject_t* xobj = NULL;
    if (context->current_obj != NULL)
    {
        // pdf_dict_t* tmp_dict = pdf_dict_get_dict(context->current_obj->value->val.dict, "/Resources");
        // tmp_dict = pdf_dict_get_dict(tmp_dict, "/XObject");
        // int ref = pdf_dict_get_ref(tmp_dict, buf);
        // tmp_obj = pdf_file_get_obj(context->page->pdf, ref);
        xobj = pdf_obj_get_xobject(context->current_obj, buf);
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
    plutovg_canvas_save(context->canvas);
    if (xobj->type == XOBJ_FORM)
    {
        plutovg_matrix_t m;
        plutovg_matrix_init(&m, xobj->form->matrix[0], xobj->form->matrix[1],
            xobj->form->matrix[2], xobj->form->matrix[3],
            xobj->form->matrix[4], xobj->form->matrix[5]);

        plutovg_canvas_transform(context->canvas, &m);
        // plutovg_canvas_move_to(context->canvas, 0, 0);
        // plutovg_canvas_rect(context->canvas, xobj->form->bbox[0],
        //     xobj->form->bbox[1], xobj->form->bbox[2],
        //     xobj->form->bbox[3]);
        // plutovg_canvas_clip(context->canvas);

        if (xobj->obj->stream != NULL)
        {
            pdf_obj_t* save_obj = context->current_obj;
            context->current_obj = xobj->obj;
            pdf_stream_open(xobj->obj->stream);
            pdf_parser_token_t* tk = NULL;
            while ((tk = pdf_stream_get_next_token(xobj->obj->stream)) != NULL)
            {
                if (tk->type == TOKEN_STREAM_END)
                    break;
                _do_render_operation(context, tk);
                pdf_parser_token_free(xobj->obj->stream->parser, tk);
            }

            pdf_stream_close(xobj->obj->stream);
            context->current_obj = save_obj;
        }
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
        // plutovg_surface_write_to_png(s, filename);

        // Scale factors to normalize image dimensions to unit space
        // plutovg_matrix_t m = { xobj->image->width, 0, 0, -xobj->image->height, 0,
        // xobj->image->height }; plutovg_matrix_invert(&m, &m);
        // plutovg_canvas_set_texture(context->canvas, s,
        // PLUTOVG_TEXTURE_TYPE_PLAIN, 1.f, &m);
        float scale_x = 1.f / xobj->image->width;
        float scale_y = 1.f / xobj->image->height;

        // Transformation matrix to scale and flip the image vertically
        plutovg_matrix_t m = { scale_x,  0,
                                0, -scale_y,
                                0, xobj->image->height * scale_y };

        plutovg_canvas_set_texture(context->canvas, s, PLUTOVG_TEXTURE_TYPE_PLAIN,
            1.0f, &m);
        plutovg_canvas_paint(context->canvas);
        plutovg_surface_destroy(s);
    }
    // pdf_page_xobject_free(xobj);
    plutovg_canvas_restore(context->canvas);
}
