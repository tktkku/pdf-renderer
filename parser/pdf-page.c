#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <assert.h>
#include <stdbool.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "plutovg-stb-image-write.h"
pdf_page_t* pdf_page_init()
{
    pdf_page_t* page = (pdf_page_t*)malloc(sizeof(pdf_page_t));
    if (page == NULL)
    {
        return NULL;
    }
    memset(page, 0, sizeof(pdf_page_t));

    return page;
}

int pdf_page_get_streams(pdf_page_t* page)
{
    if (page == NULL) return 0;

    if (page->contents == NULL) return 0;

    return page->num_contents;
}
void pdf_page_get_ext_gstate(pdf_page_t* page, const char* name)
{
    if (page == NULL)
        return;
    if (page->resources == NULL)
        return;
    if (page->resources->ext_gstate == NULL)
        return;
    pdf_dict_t* ext_gstate = pdf_dict_get_dict(page->resources->ext_gstate, name);
    if (ext_gstate == NULL)
    {
        int ref = pdf_dict_get_ref(page->resources->ext_gstate, name);
        if (ref == -1)
        {
            return;
        }
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, ref);
        ext_gstate = obj->value->val.dict;
        if (ext_gstate == NULL)
        {
            return;
        }
    }
    const char* Type = pdf_dict_get_name(ext_gstate, "/Type");// shall be ExtGState
    double LW = pdf_dict_get_number(ext_gstate, "/LW"); // line width
    int LC = pdf_dict_get_number(ext_gstate, "/LC"); // line cap
    int LJ = pdf_dict_get_number(ext_gstate, "/LJ"); // line join
    double ML = pdf_dict_get_number(ext_gstate, "/ML"); // miter limit
    pdf_array_t* D = pdf_dict_get_array(ext_gstate, "/D"); // dash pattern
    const char* RI = pdf_dict_get_name(ext_gstate, "/RI");
    int OP = pdf_dict_get_bool(ext_gstate, "/OP"); // whether to apply overprint
    int op = pdf_dict_get_bool(ext_gstate, "/op");
    int OPM = pdf_dict_get_number(ext_gstate, "/OPM");
    pdf_array_t* Font = pdf_dict_get_array(ext_gstate, "/Font");
    void* BG;
    void* BG2;
    void* UCR;
    void* UCR2;
    void* TR;
    void* TR2;
    void* HT;
    double FL = pdf_dict_get_number(ext_gstate, "/FL");
    double SM = pdf_dict_get_number(ext_gstate, "/SM");
    int SA = pdf_dict_get_bool(ext_gstate, "/SA");
    void* BM;
    void* SMask;
    double CA = pdf_dict_get_number(ext_gstate, "/CA");
    double ca = pdf_dict_get_number(ext_gstate, "/ca");
    int AIS = pdf_dict_get_bool(ext_gstate, "/AIS");
    int TK = pdf_dict_get_bool(ext_gstate, "/TK");
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
pdf_xobject_t* pdf_page_get_xobject(pdf_page_t* page, const char* name)
{
    if (page == NULL)
        return NULL;
    if (page->resources == NULL)
        return NULL;
    if (page->resources->xobject_dict == NULL)
        return NULL;
    if (name == NULL)
        return NULL;
    int ref = pdf_dict_get_ref(page->resources->xobject_dict, name);
    if (ref == -1)
        return NULL;
    // TODO: save img to img_obj
    pdf_obj_t* img_obj = pdf_file_get_obj(page->pdf, ref);
    if (img_obj == NULL)
        return NULL;
    if (img_obj->xobject != NULL)
        return img_obj->xobject;
    // unsigned char* input = img_obj->stream;
    pdf_dict_t* img_dict = img_obj->value->val.dict;
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
            if (color_space_aar->num_elements == 2
                && color_space_aar->values[0]->type == NAME
                && strcmp(color_space_aar->values[0]->val.name, "/ICCBased") == 0)
            {
                pdf_obj_t* color_space_obj = pdf_file_get_obj(page->pdf, color_space_aar->values[1]->val.indirect);
                int N = pdf_dict_get_number(color_space_obj->value->val.dict, "/N");
                char* alternate_name = pdf_dict_get_name(color_space_obj->value->val.dict, "/Alternate");
                pdf_array_t* alternate_aar = NULL;
                if (alternate_name == NULL)
                {
                    alternate_aar = pdf_dict_get_array(color_space_obj->value->val.dict, "/Alternate");
                }
                pdf_array_t* range = pdf_dict_get_array(color_space_obj->value->val.dict, "/Range");
                int length = pdf_dict_get_number(color_space_obj->value->val.dict, "/Length");
                char* filter = pdf_dict_get_name(color_space_obj->value->val.dict, "/Filter");
                if (!strcmp(filter, "/FlateDecode"))
                {
                    char* icc_data;
                    int icc_data_len = 0;
                    pdf_stream_get_all(color_space_obj->stream, &icc_data, &icc_data_len);
                    if (icc_data_len == 0)
                    {
                        return NULL;
                    }
                }
            }
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
            pdf_stream_get_all(img_obj->stream, &img->data, &img->data_len);
            if (smask_ref != -1)
            {
                pdf_obj_t* smask_obj = pdf_file_get_obj(page->pdf, smask_ref);
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
            img_obj->xobject = xobj;
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
            fseek(page->pdf->pFile, img_obj->stream->stream_offset, SEEK_SET);
            int ret = fread(img->data, 1, length, page->pdf->pFile);
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

        xobj->form->data_len = img_obj->stream->stream_len;
        xobj->form->data = (unsigned char*)malloc(xobj->form->data_len);
        //memcpy(xobj->form->data, img_obj->stream, xobj->form->data_len);

        return xobj;
    }
    return NULL;
}
uint16_t _hex_str_to_16bit(char hexStr[4])
{
    // convert every 8 bit to hex value
    uint16_t tmp[4];
    for (int i = 0; i < 4; i++)
    {
        if (hexStr[i] >= '0' && hexStr[i] <= '9')
        {
            tmp[i] = hexStr[i] - '0';
        }
        else if (hexStr[i] >= 'A' && hexStr[i] <= 'F')
        {
            tmp[i] = hexStr[i] - 'A' + 10;
        }
        else if (hexStr[i] >= 'a' && hexStr[i] <= 'f')
        {
            tmp[i] = hexStr[i] - 'a' + 10;
        }
    }
    return (
        ((tmp[0] << 12) & 0xF000)
        | ((tmp[1] << 8) & 0x0F00)
        | ((tmp[2] << 4) & 0x00F0)
        | ((tmp[3]) & 0x000F)
        );
}

uint8_t _hex_str_to_8bit(char hexStr[2])
{
    uint8_t tmp[2];
    for (int i = 0; i < 2; i++)
    {
        if (hexStr[i] >= '0' && hexStr[i] <= '9')
        {
            tmp[i] = hexStr[i] - '0';
        }
        else if (hexStr[i] >= 'A' && hexStr[i] <= 'F')
        {
            tmp[i] = hexStr[i] - 'A' + 10;
        }
        else if (hexStr[i] >= 'a' && hexStr[i] <= 'f')
        {
            tmp[i] = hexStr[i] - 'a' + 10;
        }
    }
    return (
        ((tmp[0] << 4) & 0xF0)
        |
        ((tmp[1]) & 0x0F)
        );
}

pdf_font_t* pdf_page_get_font(pdf_page_t* page, const char* name)
{
    if (page == NULL || page->resources == NULL || page->resources->font_dict == NULL || name == NULL)
        return NULL;
    int ref = pdf_dict_get_ref(page->resources->font_dict, name);
    if (ref == -1)
        return NULL;
    pdf_obj_t* font_obj = pdf_file_get_obj(page->pdf, ref);
    if (font_obj == NULL)
        return NULL;

    pdf_dict_t* font_dict = font_obj->value->val.dict;
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;


    font->type = pdf_dict_get_name(font_dict, "/Type"); // Font
    if (!font->type)
    {
        pdf_font_free(font);
        return NULL;
    }

    // Type0
    // Type1 MMType1
    // Type3
    font->subtype = pdf_dict_get_name(font_dict, "/Subtype");
    if (!font->subtype)
    {
        pdf_font_free(font);
        return NULL;
    }
    // only support Type0
    if (strcmp(font->subtype, "/Type0"))
    {
        pdf_font_free(font);
        return NULL;
    }
    // pdf_dict_get_name(font_dict, "/Name"); // not used in PDF 1.7
    font->basefont = pdf_dict_get_name(font_dict, "/BaseFont");
    font->encoding = pdf_dict_get_name(font_dict, "/Encoding");
    if (font->encoding != NULL)
    {
        pdf_cmap_t* cmap = pdf_file_get_cmap(page->pdf, font->encoding);
        if (font->cmap == NULL)
        {
            font->cmap = cmap;
        }
        else
        {
            pdf_cmap_t* t = font->cmap;
            while (t->next != NULL)
            {
                t = t->next;
            }
            t->next = cmap;
        }
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        // PDF Specification 1.7, 9.10.3 ToUnicode CMaps
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, to_unicode_ref);
        char* data = NULL;
        int len;
        pdf_stream_get_all(obj->stream, &data, &len);
        pdf_buffer_t b1 = {
            .buffer = data,
            .buffer_size = len,
            .processed = 0
        };
        pdf_parser_t* cmap_parser = pdf_parser_init(page->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(cmap_parser);
        pdf_parser_free(cmap_parser);
        cmap->worldwide = false;
        if (font->cmap == NULL)
        {
            font->cmap = cmap;
        }
        else
        {
            pdf_cmap_t* t = font->cmap;
            while (t->next != NULL)
            {
                t = t->next;
            }
            t->next = cmap;
        }
    }
    // CIDFonts
    font->descendant_font_dict = NULL;
    pdf_array_t* descendant_fonts_aar = pdf_dict_get_array(font_dict, "/DescendantFonts");
    if (descendant_fonts_aar != NULL && descendant_fonts_aar->values[0]->type == DICT)
    {
        font->descendant_font_dict = descendant_fonts_aar->values[0]->val.dict;
    }
    else if (descendant_fonts_aar != NULL && descendant_fonts_aar->values[0]->type == INDIRECT)
    {
        int descendant_fonts_ref = descendant_fonts_aar->values[0]->val.indirect;
        pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendant_fonts_ref);
        font->descendant_font_dict = descendant_font_obj->value->val.dict;
    }
    else
    {
        int descendantfonts_ref = pdf_dict_get_ref(font_dict, "/DescendantFonts");
        if (descendantfonts_ref != -1)
        {
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendantfonts_ref);
            if (descendant_font_obj != NULL && descendant_font_obj->value->type == DICT)
            {
                font->descendant_font_dict = descendant_font_obj->value->val.dict;
            }
            else if (descendant_font_obj != NULL && descendant_font_obj->value->type == ARRAY)
            {
                pdf_array_t* descendant_fonts_aar = descendant_font_obj->value->val.array;
                if (descendant_fonts_aar->values[0]->type == DICT)
                {
                    font->descendant_font_dict = descendant_fonts_aar->values[0]->val.dict;
                }
                else if (descendant_fonts_aar->values[0]->type == INDIRECT)
                {
                    int descendant_fonts_ref = descendant_fonts_aar->values[0]->val.indirect;
                    pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendant_fonts_ref);
                    font->descendant_font_dict = descendant_font_obj->value->val.dict;
                }
            }
        }
    }
    if (font->descendant_font_dict == NULL)
    {
        pdf_font_free(font);
        return NULL;
    }


    font->type = pdf_dict_get_name(font->descendant_font_dict, "/Type"); // Font
    font->subtype = pdf_dict_get_name(font->descendant_font_dict, "/Subtype"); // CIDFontType0 CIDFontType2
    // for CIDFontType0, it shall be the value of the CIDFontName entry
    // for CIDFontType2, 
    font->basefont = pdf_dict_get_name(font->descendant_font_dict, "/BaseFont");
    font->cid_system_info_ref = pdf_dict_get_ref(font->descendant_font_dict, "/CIDSystemInfo");
    if (font->cid_system_info_ref != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font->cid_system_info_ref);
        char* registry = pdf_dict_get_string(obj->value->val.dict, "/Registry");
        char* ordering = pdf_dict_get_string(obj->value->val.dict, "/Ordering");
        int supplement = pdf_dict_get_number(obj->value->val.dict, "/Supplement");
    }
    int font_descriptor_ref = pdf_dict_get_ref(font->descendant_font_dict, "/FontDescriptor");
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font_descriptor_ref);
        font->font_descriptor = obj->value->val.dict;
        font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
        font->basefont = pdf_dict_get_name(font->font_descriptor, "/FontName");
        font->font_weight = pdf_dict_get_number(font->font_descriptor, "/FontWeight");
        font->flags = pdf_dict_get_number(font->font_descriptor, "/Flags");
        font->italic_angle = pdf_dict_get_number(font->font_descriptor, "/ItalicAngle");
        font->font_bbox = pdf_dict_get_array(font->font_descriptor, "/FontBBox");
        font->ascent = pdf_dict_get_number(font->font_descriptor, "/Ascent");
        font->descent = pdf_dict_get_number(font->font_descriptor, "/Descent");
        font->cap_height = pdf_dict_get_number(font->font_descriptor, "/CapHeight");
        font->stemv = pdf_dict_get_number(font->font_descriptor, "/StemV");
        font->cid_set_ref = pdf_dict_get_ref(font->font_descriptor, "/CIDSet");

        font->fontfile1_ref = pdf_dict_get_ref(font->font_descriptor, "/FontFile1");
        if (font->fontfile1_ref != -1)
        {
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(page->pdf, font->fontfile1_ref);
            if (fontfile_obj != NULL)
            {
                if (fontfile_obj->font_data != NULL && fontfile_obj->font_data_len != 0)
                {
                    font->font_data = fontfile_obj->font_data;
                    font->font_data_length = fontfile_obj->font_data_len;
                }
                else
                {
                    pdf_stream_get_all(fontfile_obj->stream, &font->font_data, &font->font_data_length);
                    fontfile_obj->font_data = font->font_data;
                    fontfile_obj->font_data_len = font->font_data_length;
                }
            }
        }
        font->fontfile2_ref = pdf_dict_get_ref(font->font_descriptor, "/FontFile2");
        if (font->fontfile2_ref != -1)
        {
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(page->pdf, font->fontfile2_ref);
            if (fontfile_obj != NULL)
            {
                if (fontfile_obj->font_data != NULL && fontfile_obj->font_data_len != 0)
                {
                    font->font_data = fontfile_obj->font_data;
                    font->font_data_length = fontfile_obj->font_data_len;
                }
                else
                {
                    pdf_stream_get_all(fontfile_obj->stream, &font->font_data, &font->font_data_length);
                    fontfile_obj->font_data = font->font_data;
                    fontfile_obj->font_data_len = font->font_data_length;
                }
            }
        }
        font->fontfile3_ref = pdf_dict_get_ref(font->font_descriptor, "/FontFile3");
        if (font->fontfile3_ref != -1)
        {
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(page->pdf, font->fontfile3_ref);
            if (fontfile_obj != NULL)
            {
                if (fontfile_obj->font_data != NULL && fontfile_obj->font_data_len != 0)
                {
                    font->font_data = fontfile_obj->font_data;
                    font->font_data_length = fontfile_obj->font_data_len;
                }
                else
                {
                    pdf_stream_get_all(fontfile_obj->stream, &font->font_data, &font->font_data_length);
                    fontfile_obj->font_data = font->font_data;
                    fontfile_obj->font_data_len = font->font_data_length;
                }
            }
        }
    }
    // if (font->font_data != NULL)
    // {
    //     FILE* f = fopen("test.ttf", "wb+");
    //     fwrite(font->font_data, font->font_data_length, 1, f);
    //     fclose(f);
    // }
    font->dw = pdf_dict_get_number(font->descendant_font_dict, "/DW");
    if (font->dw == -1)
        font->dw = 1000;
    font->w_aar = pdf_dict_get_array(font->descendant_font_dict, "/W");
    pdf_array_t* dw2_aar = pdf_dict_get_array(font->descendant_font_dict, "/DW2");
    pdf_array_t* w2_aar = pdf_dict_get_array(font->descendant_font_dict, "/W2");
    // shall be Identity
    font->cid_to_gid_map = pdf_dict_get_name(font->descendant_font_dict, "/CIDToGIDMap");
    if (font->cid_to_gid_map == NULL)
    {
        font->cid_to_gid_map_ref = pdf_dict_get_ref(font->descendant_font_dict, "/CIDToGIDMap");
        if (font->cid_to_gid_map_ref != -1)
        {
            pdf_obj_t* cid_to_gid_obj = pdf_file_get_obj(page->pdf, font->cid_to_gid_map_ref);
            if (cid_to_gid_obj != NULL)
            {
                int size = 0;
                pdf_stream_get_all(cid_to_gid_obj->stream, &font->cid_to_gid_map, &size);
            }
        }
    }

    return font;
}
void pdf_page_xobject_free(pdf_xobject_t* xobject)
{
    if (xobject == NULL)
        return;

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
            if (xobject->form->data)
            {
                free(xobject->form->data);
                xobject->form->data = NULL;
            }
            free(xobject->form);
            xobject->form = NULL;
        }
    }
    free(xobject);
    xobject = NULL;
}

pdf_stream_t* pdf_page_get_stream(pdf_page_t* page, int index)
{
    if (page == NULL) return NULL;
    if (page->contents == NULL) return NULL;
    if (index > page->num_contents - 1) return NULL;

    pdf_obj_t* obj = page->contents[index];
    if (obj == NULL)
    {
        return false;
    }

    return obj->stream;
}

void pdf_page_free(pdf_page_t* page)
{
    if (page == NULL) return;

    if (page->resources)
    {
        free(page->resources);
        page->resources = NULL;
    }

    if (page->contents)
    {
        free(page->contents);
        page->contents = NULL;
    }

    free(page);
    page = NULL;
}

int pdf_page_get_media_width(pdf_page_t* page)
{
    if (page == NULL) return -1;
    return page->media_box.width;
}
int pdf_page_get_media_height(pdf_page_t* page)
{
    if (page == NULL) return -1;
    return page->media_box.height;
}