#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "plutovg-stb-image-write.h"
pdf_obj_t* pdf_obj_init()
{
    pdf_obj_t* obj = (pdf_obj_t*)malloc(sizeof(pdf_obj_t));
    memset(obj, 0, sizeof(pdf_obj_t));

    return obj;
}

void pdf_obj_free(pdf_obj_t* obj)
{
    if (obj == NULL)
    {
        return;
    }
    if (obj->stream != NULL)
    {
        pdf_stream_free(obj->stream);
    }
    if (obj->xobject != NULL)
    {
        pdf_page_xobject_free(obj->xobject);
    }
    if (obj->font_data != NULL)
    {
        free(obj->font_data);
    }
    pdf_value_free(obj->value);
    obj->value = NULL;
    free(obj);
    obj = NULL;
}
void _write_png_callback(void* context, void* data, int size)
{
    pdf_image_t* img = (pdf_image_t*)context;

    unsigned char* t = (unsigned char*)realloc(img->data, img->data + size);
    if (t == NULL)
    {
        free(img->data);
        img->data = NULL;
        img->data_len = 0;
        return;
    }
    img->data = t;
    memcpy(img->data + img->data_len, data, size);
    img->data_len += size;
}

pdf_xobject_t* pdf_obj_get_xobject(pdf_obj_t* obj)
{
    if (obj == NULL)
        return NULL;
    if (obj->xobject != NULL)
        return obj->xobject;
    // unsigned char* input = img_obj->stream;
    pdf_dict_t* img_dict = obj->value->val.dict;
    const char* type = pdf_dict_get_name(img_dict, "/Type"); // XObject
    if (strcmp(type, "/XObject") != 0)
    {
        return NULL;
    }
    const char* subtype = pdf_dict_get_name(img_dict, "/Subtype");
    const char* subtype2 = pdf_dict_get_name(img_dict, "/Subtype2");
    if (strcmp(subtype, "/PS") == 0 || (subtype2 != NULL && !strcmp(subtype, "/Form") && !strcmp(subtype2, "/PS")))
    {
        // not used
    }
    else if (strcmp(subtype, "/Image") == 0)
    {
        // the value shall be one of 1 2 4 8 16
        // if ImageMask is true, this entry is optional, but if specified, its value shall be 1
        int bits_per_component = pdf_dict_get_number(img_dict, "/BitsPerComponent");
        const char* filter = pdf_dict_get_name(img_dict, "/Filter");
        pdf_array_t* filter_arr = NULL;
        if (filter == NULL)
        {
            filter_arr = pdf_dict_get_array(img_dict, "/Filter");
            if (filter_arr == NULL)
            {
                return NULL;
            }

            if (filter_arr->num_elements == 1)
            {
                filter = filter_arr->values[0]->val.name;
            }
        }
        int width = pdf_dict_get_number(img_dict, "/Width");
        int height = pdf_dict_get_number(img_dict, "/Height");
        int length = pdf_dict_get_number(img_dict, "/Length");
        const char* color_space = pdf_dict_get_name(img_dict, "/ColorSpace");
        const char* name = pdf_dict_get_name(img_dict, "/Intent");
        pdf_array_t* mask_arr = pdf_dict_get_array(img_dict, "/Mask");
        pdf_array_t* decode_aar = pdf_dict_get_array(img_dict, "/Decode");
        int interpolate = pdf_dict_get_bool(img_dict, "/Interpolate");
        pdf_array_t* alter_aar = pdf_dict_get_array(img_dict, "/Alternates");
        int smask_ref = pdf_dict_get_ref(img_dict, "/SMask");
        int smask_in_data = pdf_dict_get_number(img_dict, "/SMaskInData");
        const char* metadata = pdf_dict_get_name(img_dict, "/Metadata");
        pdf_dict_t* oc_dict = pdf_dict_get_dict(img_dict, "/OC");
        pdf_array_t* color_space_aar = NULL;
        if (color_space == NULL)
        {
            color_space_aar = pdf_dict_get_array(img_dict, "/ColorSpace");
        }
        int imageMask = pdf_dict_get_bool(img_dict, "/ImageMask");
        if (imageMask > 0)
        {

        }
        if (strcmp(filter, "/FlateDecode") == 0)
        {
            pdf_image_t* img = calloc(1, sizeof(pdf_image_t));
            img->width = width;
            img->height = height;
            img->bits_per_color = bits_per_component;
            if (color_space != NULL)
            {
                strcpy(img->color_space, color_space);
            }
            //int image_size = width * height * (bit_count / 8);
            // img->data = (unsigned char*)calloc(image_size, sizeof(unsigned char));
            // //memcpy(img->data, input, img_obj->stream_len);
            // img->data_len = image_size;
            pdf_stream_get_all(obj->stream, &img->data, &img->data_len);
            if (smask_ref != -1)
            {
                pdf_obj_t* smask_obj = pdf_file_get_obj(obj->pdf, smask_ref);
                if (smask_obj != NULL)
                {
                    unsigned char* smask = NULL;
                    int smask_len = 0;
                    pdf_stream_get_all(smask_obj->stream, &smask, &smask_len);
                    if (smask != NULL)
                    {
                        int tmp_len = sizeof(unsigned char) * width * 4 * height;
                        unsigned char* tmp = (unsigned char*)malloc(tmp_len);
                        for (int i = 0; i < height; i++)
                        {
                            for (int j = 0; j < width; j++)
                            {
                                int index = (i * width + j);
                                int index1 = index * 3;
                                int index2 = index * 4;
                                tmp[index2] = img->data[index1];
                                tmp[index2 + 1] = img->data[index1 + 1];
                                tmp[index2 + 2] = img->data[index1 + 2];
                                tmp[index2 + 3] = smask[index];
                            }
                        }
                        pdf_image_t t = {
                            .data = NULL,
                            .data_len = 0
                        };
                        int success = stbi_write_png_to_func(_write_png_callback, &t,
                            width, height, 4, tmp, width * 4);
                        if (!success || t.data == NULL)
                        {
                            free(t.data);
                        }
                        else
                        {
                            free(img->data);
                            img->data = t.data;
                            img->data_len = t.data_len;
                        }
                        free(tmp);
                    }
                }
            }
            pdf_xobject_t* xobj = (pdf_xobject_t*)malloc(sizeof(pdf_xobject_t));
            xobj->type = XOBJ_IMAGE;
            xobj->image = img;
            obj->xobject = xobj;
            return xobj;
        }
        else if (strcmp(filter, "/DCTDecode") == 0)
        {
            pdf_image_t* img = calloc(1, sizeof(pdf_image_t));
            img->width = width;
            img->height = height;
            img->bits_per_color = bits_per_component;
            img->data = calloc(length, sizeof(char));
            if (color_space != NULL)
            {
                strcpy(img->color_space, color_space);
            }
            fseek(obj->pdf->pFile, obj->stream->stream_offset, SEEK_SET);
            int ret = fread(img->data, 1, length, obj->pdf->pFile);
            if (ret != length)
            {
                free(img);
                return NULL;
            }
            img->data_len = length;

            pdf_xobject_t* xobj = (pdf_xobject_t*)malloc(sizeof(pdf_xobject_t));
            xobj->type = XOBJ_IMAGE;
            xobj->image = img;
            return xobj;
        }
        else if (strcmp(filter, "/JPXDecode") == 0)
        {
            // 1. If ColorSpace is present, any colour space specifications in the JPEG2000 data shall be ignored
            // 2. If ColorSpace is absent, the colour space specifications in the JPGE2000 data shall be used.
            // The Decode array shall also be ignored unless ImageMask is true  
        }
    }
    else if (strcmp(subtype, "/Form") == 0)
    {
        pdf_xobject_t* xobj = (pdf_xobject_t*)malloc(sizeof(pdf_xobject_t));
        xobj->type = XOBJ_FORM;
        xobj->form = (pdf_form_t*)malloc(sizeof(pdf_form_t));

        pdf_array_t* ctm_aar = pdf_dict_get_array(img_dict, "/Matrix");
        for (int i = 0; i < ctm_aar->num_elements; i++)
        {
            xobj->form->matrix[i] = ctm_aar->values[i]->val.number;
        }
        pdf_array_t* bbox_aar = pdf_dict_get_array(img_dict, "/BBox");
        for (int i = 0; i < bbox_aar->num_elements; i++)
        {
            xobj->form->bbox[i] = bbox_aar->values[i]->val.number;
        }
        obj->xobject = xobj;
        return xobj;
    }
    return NULL;
}