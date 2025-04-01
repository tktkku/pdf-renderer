#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "plutovg-stb-image-write.h"

PdfObj::~PdfObj()
{
    if (stream != nullptr)
    {
        delete stream;
    }
    if (xobject != nullptr)
    {
        if (xobject->type == XOBJ_IMAGE)
        {
            if (xobject->image)
            {
                if (xobject->image->data)
                {
                    free(xobject->image->data);
                    xobject->image->data = NULL;
                }

                free(xobject->image);
                xobject->image = NULL;
            }

        }
        else if (xobject->type == XOBJ_FORM)
        {
            if (xobject->form)
            {
                free(xobject->form);
                xobject->form = NULL;
            }
        }
        free(xobject);
    }
    if (font_data != nullptr)
    {
        free(font_data);
    }
    delete value;
}
void _write_png_callback(void* context, void* data, int size)
{
    pdf_image_t* img = (pdf_image_t*)context;

    unsigned char* t = (unsigned char*)realloc(img->data, img->data_len + size);
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

pdf_xobject_t* PdfObj::getXobject()
{
    if (xobject != nullptr)
        return xobject;
    // unsigned char* input = img_obj->stream;
    auto& img_dict = *(value->dict);
    const char* type = img_dict["/Type"].name; // XObject
    if (strcmp(type, "/XObject") != 0)
    {
        return NULL;
    }
    const char* subtype = img_dict["/Subtype"].name;
    const char* subtype2 = NULL;
    if (img_dict["/Subtype2"].type == NAME)
    {
        subtype2 = img_dict["/Subtype2"].name;
    }
    if (strcmp(subtype, "/PS") == 0 || (subtype2 != NULL && !strcmp(subtype, "/Form") && !strcmp(subtype2, "/PS")))
    {
        // not used
    }
    else if (strcmp(subtype, "/Image") == 0)
    {
        // the value shall be one of 1 2 4 8 16
        // if ImageMask is true, this entry is optional, but if specified, its value shall be 1
        int bits_per_component = img_dict["/BitsPerComponent"].number;
        const char* filter = img_dict["/Filter"].name;
        PdfArray* filter_arr = NULL;
        if (filter == NULL)
        {
            filter_arr = img_dict["/Filter"].array;
            if (filter_arr == NULL)
            {
                return NULL;
            }

            if (filter_arr->size() == 1)
            {
                filter = (*filter_arr)[0]->name;
            }
        }
        int width = img_dict["/Width"].number;
        int height = img_dict["/Height"].number;
        int length = 0;
        if (img_dict["/Length"].type == NUMBER)
        {
            length = img_dict["/Length"].number;
        }
        else if (img_dict["/Length"].type == INDIRECT)
        {
            int ref = img_dict["/Length"].indirect;
            PdfObj* len_obj = pdf_file_get_obj(pdf, ref);
            if (len_obj != NULL)
            {
                length = len_obj->value->number;
            }
        }
        const char* color_space = NULL;
        PdfArray* color_space_aar = NULL;
        if (img_dict["/ColorSpace"].type == NAME)
        {
            color_space = img_dict["/ColorSpace"].name;
        }
        else if (img_dict["/ColorSpace"].type == ARRAY)
        {
            color_space_aar = img_dict["/ColorSpace"].array;
        }
        const char* name = img_dict["/Intent"].name;
        PdfArray* mask_arr = img_dict["/Mask"].array;
        PdfArray* decode_aar = img_dict["/Decode"].array;
        int interpolate = img_dict["/Interpolate"].boolean;
        PdfArray* alter_aar = img_dict["/Alternates"].array;
        int smask_ref = -1;
        if (img_dict["/SMask"].type == INDIRECT)
        {
            smask_ref = img_dict["/SMask"].indirect;
        }
        int smask_in_data = img_dict["/SMaskInData"].number;
        const char* metadata = img_dict["/Metadata"].name;
        PdfDict* oc_dict = img_dict["/OC"].dict;

        int imageMask = img_dict["/ImageMask"].boolean;
        if (imageMask > 0)
        {

        }
        if (strcmp(filter, "/FlateDecode") == 0)
        {
            pdf_image_t* img = (pdf_image_t*)calloc(1, sizeof(pdf_image_t));
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
            stream->getAll(&img->data, &img->data_len);
            if (smask_ref != -1)
            {
                PdfObj* smask_obj = pdf_file_get_obj(pdf, smask_ref);
                if (smask_obj != NULL)
                {
                    unsigned char* smask = NULL;
                    int smask_len = 0;
                    smask_obj->stream->getAll(&smask, &smask_len);
                    if (smask != NULL)
                    {
                        int tmp_len = sizeof(unsigned char) * width * 4 * height;
                        unsigned char* tmp = (unsigned char*)malloc(tmp_len);
                        int stride = img->data_len / height;
                        if (color_space_aar != NULL && !strcmp((*color_space_aar)[0]->name, "/Indexed"))
                        {
                            int lookup_cnt = (*color_space_aar)[2]->number;
                            char* lookup = (*color_space_aar)[3]->string + 1;
                            for (int i = 0; i < height; i++)
                            {
                                for (int j = 0; j < width; j++)
                                {
                                    int index = (i * width + j);
                                    int index1 = img->data[i * stride + j] * 3;
                                    int index2 = index * 4;

                                    tmp[index2] = lookup[index1];
                                    tmp[index2 + 1] = lookup[index1 + 1];
                                    tmp[index2 + 2] = lookup[index1 + 2];
                                    tmp[index2 + 3] = smask[index];
                                }
                            }
                        }
                        else
                        {
                            for (int i = 0; i < height; i++)
                            {
                                for (int j = 0; j < width; j++)
                                {
                                    int index = (i * width + j);
                                    int index1 = i * stride + j * 3;
                                    int index2 = index * 4;
                                    tmp[index2] = img->data[index1];
                                    tmp[index2 + 1] = img->data[index1 + 1];
                                    tmp[index2 + 2] = img->data[index1 + 2];
                                    tmp[index2 + 3] = smask[index];
                                }
                            }
                        }
                        pdf_image_t t;
                        t.data = NULL;
                        t.data_len = 0;
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
            this->xobject = xobj;
            return xobj;
        }
        else if (strcmp(filter, "/DCTDecode") == 0)
        {
            pdf_image_t* img = (pdf_image_t*)calloc(1, sizeof(pdf_image_t));
            img->width = width;
            img->height = height;
            img->bits_per_color = bits_per_component;
            img->data = (unsigned char*)calloc(length, sizeof(char));
            if (color_space != NULL)
            {
                strcpy(img->color_space, color_space);
            }
            fseek(pdf->pFile, stream->stream_offset, SEEK_SET);
            int ret = fread(img->data, 1, length, pdf->pFile);
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

        if (img_dict["/Matrix"].type == ARRAY)
        {
            PdfArray* ctm_aar = img_dict["/Matrix"].array;
            for (int i = 0; ctm_aar && i < ctm_aar->size(); i++)
            {
                xobj->form->matrix[i] = (*ctm_aar)[i]->number;
            }
        }

        if (img_dict["/BBox"].type == ARRAY)
        {
            PdfArray* bbox_aar = img_dict["/BBox"].array;
            for (int i = 0; bbox_aar && i < bbox_aar->size(); i++)
            {
                xobj->form->bbox[i] = (*bbox_aar)[i]->number;
            }
            this->xobject = xobj;
        }

        return xobj;
    }
    return nullptr;
}