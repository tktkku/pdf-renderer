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
        if (obj->xobject->type == XOBJ_IMAGE)
        {
            if (obj->xobject->image)
            {
                if (obj->xobject->image->data)
                {
                    free(obj->xobject->image->data);
                    obj->xobject->image->data = NULL;
                }
    
                free(obj->xobject->image);
                obj->xobject->image = NULL;
            }
    
        }
        else if (obj->xobject->type == XOBJ_FORM)
        {
            if (obj->xobject->form)
            {
                free(obj->xobject->form);
                obj->xobject->form = NULL;
            }
        }
        free(obj->xobject);
    }
    if (obj->font != NULL)
    {
        pdf_font_free(obj->font);
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
void pdf_obj_get_extgstate(pdf_obj_t* obj, const char* name)
{

}
pdf_font_t* _load_type0_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    // pdf_dict_get_name(font_dict, "/Name"); // not used in PDF 1.7
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");
    if (font->encoding != NULL)
    {
        pdf_cmap_t* cmap = pdf_file_get_cmap(obj->pdf, font->encoding);
        font->cmap = cmap;
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        // PDF Specification 1.7, 9.10.3 ToUnicode CMaps
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        unsigned char* origin = data;
        pdf_parser_t* parser = pdf_parser_init(obj->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
        free(origin);
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
        pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendant_fonts_ref);
        font->descendant_font_dict = descendant_font_obj->value->val.dict;
    }
    else
    {
        int descendantfonts_ref = pdf_dict_get_ref(font_dict, "/DescendantFonts");
        if (descendantfonts_ref != -1)
        {
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendantfonts_ref);
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
                    pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendant_fonts_ref);
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

    font->type = (char*)pdf_dict_get_name(font->descendant_font_dict, "/Type"); // Font
    font->subtype = (char*)pdf_dict_get_name(font->descendant_font_dict, "/Subtype"); // CIDFontType0 CIDFontType2
    // for CIDFontType0, it shall be the value of the CIDFontName entry
    // for CIDFontType2, 
    font->basefont = (char*)pdf_dict_get_name(font->descendant_font_dict, "/BaseFont");
    font->cid_system_info_ref = pdf_dict_get_ref(font->descendant_font_dict, "/CIDSystemInfo");
    if (font->cid_system_info_ref != -1)
    {
        // pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font->cid_system_info_ref);
        // char* registry = pdf_dict_get_string(obj->value->val.dict, "/Registry");
        // char* ordering = pdf_dict_get_string(obj->value->val.dict, "/Ordering");
        // int supplement = pdf_dict_get_number(obj->value->val.dict, "/Supplement");
    }
    int font_descriptor_ref = pdf_dict_get_ref(font->descendant_font_dict, "/FontDescriptor");
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, font_descriptor_ref);
        font->font_descriptor = obj1->value->val.dict;
    }
    else
    {
        font->font_descriptor = pdf_dict_get_dict(font->descendant_font_dict, "/FontDescriptor");
    }
    if (font->font_descriptor != NULL)
    {
        // font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
        font->basefont = (char*)pdf_dict_get_name(font->font_descriptor, "/FontName");
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile1_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile2_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile3_ref);
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
    //pdf_array_t* dw2_aar = pdf_dict_get_array(font->descendant_font_dict, "/DW2");
    //pdf_array_t* w2_aar = pdf_dict_get_array(font->descendant_font_dict, "/W2");
    // shall be Identity
    font->cid_to_gid_map = (unsigned char*)pdf_dict_get_name(font->descendant_font_dict, "/CIDToGIDMap");
    if (font->cid_to_gid_map == NULL)
    {
        font->cid_to_gid_map_ref = pdf_dict_get_ref(font->descendant_font_dict, "/CIDToGIDMap");
        if (font->cid_to_gid_map_ref != -1)
        {
            pdf_obj_t* cid_to_gid_obj = pdf_file_get_obj(obj->pdf, font->cid_to_gid_map_ref);
            if (cid_to_gid_obj != NULL)
            {
                int size = 0;
                pdf_stream_get_all(cid_to_gid_obj->stream, &font->cid_to_gid_map, &size);
            }
        }
    }

    return font;
}
pdf_font_t* _load_truetype_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->type = (char*)pdf_dict_get_name(font_dict, "/Type"); // Font
    font->subtype = (char*)pdf_dict_get_name(font_dict, "/Subtype");
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->first_char = pdf_dict_get_number(font_dict, "/FirstChar");
    font->last_char = pdf_dict_get_number(font_dict, "/LastChar");
    font->widths = pdf_dict_get_array(font_dict, "/Widths");
    font->encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        unsigned char* origin = data;
        pdf_parser_t* parser = pdf_parser_init(obj->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
        free(origin);
    }
    int font_descriptor_ref = pdf_dict_get_ref(font_dict, "/FontDescriptor");
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, font_descriptor_ref);
        font->font_descriptor = obj1->value->val.dict;
    }
    else
    {
        font->font_descriptor = pdf_dict_get_dict(font_dict, "/FontDescriptor");
    }
    if (font->font_descriptor != NULL)
    {
        // font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
        font->basefont = (char*)pdf_dict_get_name(font->font_descriptor, "/FontName");
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile1_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile2_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile3_ref);
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
    return font;
}
pdf_font_t* _load_type3_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->type = (char*)pdf_dict_get_name(font_dict, "/Type"); // Font
    font->subtype = (char*)pdf_dict_get_name(font_dict, "/Subtype");
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->first_char = pdf_dict_get_number(font_dict, "/FirstChar");
    font->last_char = pdf_dict_get_number(font_dict, "/LastChar");
    font->widths = pdf_dict_get_array(font_dict, "/Widths");
    if (font->widths == NULL)
    {
        int ref = pdf_dict_get_ref(font_dict, "/Widths");
        if (ref != -1)
        {
            pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
            if (obj1 != NULL)
            {
                font->widths = obj1->value->val.array;
            }
        }
    }
    font->font_matrix = pdf_dict_get_array(font_dict, "/FontMatrix");
    font->font_bbox = pdf_dict_get_array(font_dict, "/FontBBox");
    font->charProcs = pdf_dict_get_dict(font_dict, "/CharProcs");
    if (font->charProcs == NULL)
    {
        int ref = pdf_dict_get_ref(font_dict, "/CharProcs");
        if (ref != -1)
        {
            pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
            if (obj1 != NULL)
            {
                font->charProcs = obj1->value->val.dict;
            }
        }
    }
    font->encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    if (font->encoding == NULL)
    {
        font->encoding_dict = pdf_dict_get_dict(font_dict, "/Encoding");
        if (font->encoding_dict != NULL)
        {
            pdf_array_t* arr = pdf_dict_get_array(font->encoding_dict, "/Differences");
            if (arr != NULL)
            {
                font->differences = pdf_array_init();
                font->differences->num_elements = arr->num_elements * 2;
                font->differences->values = (pdf_array_element_value_t**)malloc(font->differences->num_elements * sizeof(pdf_array_element_value_t*));
                int cnt = 0;
                for (int i = 0; i < arr->num_elements; i++)
                {
                    if (arr->values[i]->type == NUMBER)
                    {
                        font->differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                        font->differences->values[cnt]->type = NUMBER;
                        font->differences->values[cnt]->val.number = arr->values[i]->val.number;
                        cnt++;
                    }
                    else if (arr->values[i]->type == NAME)
                    {
                        if (cnt > 1 && font->differences->values[cnt - 1]->type == NAME)
                        {
                            font->differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                            font->differences->values[cnt]->type = NUMBER;
                            font->differences->values[cnt]->val.number = font->differences->values[cnt - 2]->val.number + 1;
                            cnt++;
                        }
                        font->differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                        font->differences->values[cnt]->type = NAME;
                        font->differences->values[cnt]->val.name = strdup(arr->values[i]->val.name);
                        cnt++;
                    }
                }
                pdf_array_element_value_t** tmp = (pdf_array_element_value_t**)realloc(font->differences->values, cnt * sizeof(pdf_array_element_value_t*));
                if (tmp != NULL)
                {
                    font->differences->values = tmp;
                    font->differences->num_elements = cnt;
                }
                else
                {
                    pdf_array_free(font->differences);
                    font->differences = NULL;
                }
            }
        }
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        unsigned char* origin = data;
        pdf_parser_t* parser = pdf_parser_init(obj->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
        free(origin);
    }
    int font_descriptor_ref = pdf_dict_get_ref(font_dict, "/FontDescriptor");
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, font_descriptor_ref);
        font->font_descriptor = obj1->value->val.dict;
    }
    else
    {
        font->font_descriptor = pdf_dict_get_dict(font_dict, "/FontDescriptor");
    }
    if (font->font_descriptor != NULL)
    {
        // font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
        font->basefont = (char*)pdf_dict_get_name(font->font_descriptor, "/FontName");
        font->font_weight = pdf_dict_get_number(font->font_descriptor, "/FontWeight");
        font->flags = pdf_dict_get_number(font->font_descriptor, "/Flags");
        font->italic_angle = pdf_dict_get_number(font->font_descriptor, "/ItalicAngle");
        if (font->font_bbox == NULL)
        {
            font->font_bbox = pdf_dict_get_array(font->font_descriptor, "/FontBBox");
        }
        if (font->font_matrix == NULL)
        {
            font->font_matrix = pdf_dict_get_array(font->font_descriptor, "/FontMatrix");
        }
        font->ascent = pdf_dict_get_number(font->font_descriptor, "/Ascent");
        font->descent = pdf_dict_get_number(font->font_descriptor, "/Descent");
        font->cap_height = pdf_dict_get_number(font->font_descriptor, "/CapHeight");
        font->stemv = pdf_dict_get_number(font->font_descriptor, "/StemV");
        font->cid_set_ref = pdf_dict_get_ref(font->font_descriptor, "/CIDSet");

        font->fontfile1_ref = pdf_dict_get_ref(font->font_descriptor, "/FontFile1");
        if (font->fontfile1_ref != -1)
        {
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile1_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile2_ref);
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
            pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, font->fontfile3_ref);
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
    return font;
}
void pdf_obj_get_colorspace(pdf_obj_t* obj, const char* name, char* value)
{
    if (obj == NULL || name == NULL || value == NULL || obj->resources.colorspace_dict == NULL)
    {
        return;
    }
    int ref = pdf_dict_get_ref(obj->resources.colorspace_dict, name);
    if (ref == -1)
    {
        char* color_space = pdf_dict_get_name(obj->resources.colorspace_dict, name);
        if (color_space != NULL)
        {
            strcpy(value, color_space);
        }
    }
    else
    {
        pdf_obj_t* color_space_obj = pdf_file_get_obj(obj->pdf, ref);
        if (color_space_obj != NULL)
        {
            if (color_space_obj->value->type == NAME)
            {
                strcpy(value, color_space_obj->value->val.name);
            }
            // else if (color_space_obj->value->type == ARRAY)
            // {
            //     pdf_array_t* color_space_aar = color_space_obj->value->val.array;
            //     if (color_space_aar != NULL && color_space_aar->num_elements > 0)
            //     {
            //         strcpy(value, color_space_aar->values[0]->val.name);
            //     }
            // }
            // else if (color_space_obj->value->type == DICT)
            // {
            //     pdf_dict_t* color_space_dict = color_space_obj->value->val.dict;
            //     if (color_space_dict != NULL)
            //     {
            //         strcpy(value, pdf_dict_get_name(color_space_dict, "/Name"));
            //     }
            // }
        }
    }

}
pdf_font_t* pdf_obj_get_font(pdf_obj_t* obj, const char* name)
{
    if (obj == NULL || name == NULL || obj->resources.font_dict == NULL) return NULL;
    int ref = pdf_dict_get_ref(obj->resources.font_dict, name);
    if (ref == -1)
        return NULL;
    pdf_obj_t* font_obj = pdf_file_get_obj(obj->pdf, ref);
    if (font_obj == NULL)
        return NULL;
    else if (font_obj->font != NULL)
    {
        return font_obj->font;
    }
    pdf_dict_t* font_dict = font_obj->value->val.dict;

    char* type = (char*)pdf_dict_get_name(font_dict, "/Type"); // Font
    if (!type)
    {
        return NULL;
    }
    // Type0
    // Type1 MMType1
    // Type3

    char* subtype = (char*)pdf_dict_get_name(font_dict, "/Subtype");
    if (!subtype)
    {
        return NULL;
    }

    if (!strcmp(subtype, "/TrueType"))
    {
        font_obj->font = _load_truetype_font(obj, font_dict);
    }
    else if (!strcmp(subtype, "/Type3"))
    {
        font_obj->font = _load_type3_font(obj, font_dict);
    }
    else if (!strcmp(subtype, "/Type1"))
    {
        return NULL;
    }
    else if (!strcmp(subtype, "/Type0"))
    {
        font_obj->font = _load_type0_font(obj, font_dict);
    }
    return font_obj->font;
}

pdf_xobject_t* pdf_obj_get_xobject(pdf_obj_t* obj, const char* name)
{
    if (obj == NULL || obj->resources.xobject_dict == NULL || name == NULL)
        return NULL;
    // if (obj->xobject != NULL)
    //     return obj->xobject;
    // unsigned char* input = img_obj->stream;
    pdf_obj_t* xobject = NULL;
    pdf_dict_t* xobject_dict = NULL;
    int ref = pdf_dict_get_ref(obj->resources.xobject_dict, name);
    if (ref != -1)
    {
        xobject = pdf_file_get_obj(obj->pdf, ref);
        if (xobject == NULL) return NULL;
        else if (xobject->xobject != NULL)
        {
            return xobject->xobject;
        }
        xobject_dict = xobject->value->val.dict;
    }
    else
    {
        xobject_dict = pdf_dict_get_dict(obj->resources.xobject_dict, name);
    }
    
    if (xobject_dict == NULL) return NULL;
    const char* type = pdf_dict_get_name(xobject_dict, "/Type"); // XObject
    // if (strcmp(type, "/XObject") != 0)
    // {
    //     return NULL;
    // }
    const char* subtype = pdf_dict_get_name(xobject_dict, "/Subtype");
    const char* subtype2 = pdf_dict_get_name(xobject_dict, "/Subtype2");
    if (strcmp(subtype, "/PS") == 0 || (subtype2 != NULL && !strcmp(subtype, "/Form") && !strcmp(subtype2, "/PS")))
    {
        // not used
    }
    else if (strcmp(subtype, "/Image") == 0)
    {
        // the value shall be one of 1 2 4 8 16
        // if ImageMask is true, this entry is optional, but if specified, its value shall be 1
        int bits_per_component = pdf_dict_get_number(xobject_dict, "/BitsPerComponent");
        const char* filter = pdf_dict_get_name(xobject_dict, "/Filter");
        pdf_array_t* filter_arr = NULL;
        if (filter == NULL)
        {
            filter_arr = pdf_dict_get_array(xobject_dict, "/Filter");
            if (filter_arr == NULL)
            {
                return NULL;
            }

            if (filter_arr->num_elements == 1)
            {
                filter = filter_arr->values[0]->val.name;
            }
        }
        int width = pdf_dict_get_number(xobject_dict, "/Width");
        int height = pdf_dict_get_number(xobject_dict, "/Height");
        int length = pdf_dict_get_number(xobject_dict, "/Length");
        if (length == -1)
        {
            int ref = pdf_dict_get_ref(xobject_dict, "/Length");
            if (ref != -1)
            {
                pdf_obj_t* l_obj = pdf_file_get_obj(obj->pdf, ref);
                if (l_obj)
                {
                    length = l_obj->value->val.number;
                }
            }
        }
        if (length <= 0) return NULL;
        const char* color_space = pdf_dict_get_name(xobject_dict, "/ColorSpace");
        //const char* name = pdf_dict_get_name(img_dict, "/Intent");
        //pdf_array_t* mask_arr = pdf_dict_get_array(img_dict, "/Mask");
        //pdf_array_t* decode_aar = pdf_dict_get_array(img_dict, "/Decode");
        //int interpolate = pdf_dict_get_bool(img_dict, "/Interpolate");
        //pdf_array_t* alter_aar = pdf_dict_get_array(img_dict, "/Alternates");
        int smask_ref = pdf_dict_get_ref(xobject_dict, "/SMask");
        //int smask_in_data = pdf_dict_get_number(img_dict, "/SMaskInData");
        //const char* metadata = pdf_dict_get_name(img_dict, "/Metadata");
        //pdf_dict_t* oc_dict = pdf_dict_get_dict(img_dict, "/OC");
        pdf_array_t* color_space_aar = NULL;
        if (color_space == NULL)
        {
            color_space_aar = pdf_dict_get_array(xobject_dict, "/ColorSpace");
        }
        int imageMask = pdf_dict_get_bool(xobject_dict, "/ImageMask");
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
            pdf_stream_get_all(xobject->stream, &img->data, &img->data_len);
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
                        int stride = img->data_len / height;
                        if (color_space_aar != NULL && !strcmp(color_space_aar->values[0]->val.name, "/Indexed"))
                        {
                            int lookup_cnt = color_space_aar->values[2]->val.number;
                            unsigned char* lookup = NULL;
                            if (color_space_aar->values[3]->type == INDIRECT)
                            {
                                int ref = color_space_aar->values[3]->val.indirect;
                                pdf_obj_t* lookup_obj = pdf_file_get_obj(obj->pdf, ref);
                                pdf_stream_get_all(lookup_obj->stream, &lookup, &lookup_cnt);
                            }
                            else
                            {
                                lookup = (unsigned char*)(color_space_aar->values[3]->val.string + 1);
                            }
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
                            free(lookup);
                        }
                        else
                        {
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
                        free(smask);
                        free(tmp);
                    }
                }
            }
            else if (color_space_aar != NULL && !strcmp(color_space_aar->values[0]->val.name, "/Indexed"))
            {
                int stride = img->data_len / height;
                int tmp_len = sizeof(unsigned char) * width * 3 * height;
                unsigned char* tmp = (unsigned char*)malloc(tmp_len);
                int lookup_cnt = color_space_aar->values[2]->val.number;
                unsigned char* lookup = NULL;
                if (color_space_aar->values[3]->type == INDIRECT)
                {
                    int ref = color_space_aar->values[3]->val.indirect;
                    pdf_obj_t* lookup_obj = pdf_file_get_obj(obj->pdf, ref);
                    pdf_stream_get_all(lookup_obj->stream, &lookup, &lookup_cnt);
                }
                else
                {
                    lookup = (unsigned char*)(color_space_aar->values[3]->val.string + 1);
                }
                if (bits_per_component == 8)
                {
                    for (int i = 0; i < height; i++)
                    {
                        for (int j = 0; j < width; j++)
                        {
                            int index = (i * width + j);
                            int index1 = img->data[i * stride + j] * 3;
                            int index2 = index * 3;

                            tmp[index2] = lookup[index1];
                            tmp[index2 + 1] = lookup[index1 + 1];
                            tmp[index2 + 2] = lookup[index1 + 2];
                        }
                    }
                }
                else if (bits_per_component == 4)
                {
                    for (int i = 0; i < height; i++)
                    {
                        for (int j = 0; j < stride; j++)
                        {
                            int val = img->data[i * stride + j];
                            int valh = ((val >> 4) & 0x0F) * 3;
                            int vall = ((val) & 0x0F) * 3;
                            int index = (i * width + j * 2) * 3;
                            tmp[index] = lookup[valh];
                            tmp[index + 1] = lookup[valh + 1];
                            tmp[index + 2] = lookup[valh + 2];
                            tmp[index + 3] = lookup[vall];
                            tmp[index + 4] = lookup[vall + 1];
                            tmp[index + 5] = lookup[vall + 2];
                        }
                    }
                }
                free(lookup);
                free(img->data);
                img->data = tmp;
                img->data_len = tmp_len;
                img->bits_per_color = 8;
            }
            pdf_xobject_t* xobj = (pdf_xobject_t*)malloc(sizeof(pdf_xobject_t));
            xobj->obj = xobject;
            xobj->type = XOBJ_IMAGE;
            xobj->image = img;
            // obj->xobject = xobj;
            xobject->xobject = xobj;
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
            fseek(xobject->pdf->pFile, xobject->stream->stream_offset, SEEK_SET);
            int ret = fread(img->data, 1, length, xobject->pdf->pFile);
            if (ret != length)
            {
                free(img);
                return NULL;
            }
            img->data_len = length;

            pdf_xobject_t* xobj = (pdf_xobject_t*)malloc(sizeof(pdf_xobject_t));
            xobj->obj = xobject;
            xobj->type = XOBJ_IMAGE;
            xobj->image = img;
            xobject->xobject = xobj;
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
        xobj->obj = xobject;
        xobj->type = XOBJ_FORM;
        xobj->form = (pdf_form_t*)malloc(sizeof(pdf_form_t));
        xobj->form->matrix[0] = 1;
        xobj->form->matrix[1] = 0;
        xobj->form->matrix[2] = 0;
        xobj->form->matrix[3] = 1;
        xobj->form->matrix[4] = 0;
        xobj->form->matrix[5] = 0;
        xobj->form->bbox[0] = 0;
        xobj->form->bbox[1] = 0;
        xobj->form->bbox[2] = 0;
        xobj->form->bbox[3] = 0;
        pdf_array_t* ctm_aar = pdf_dict_get_array(xobject_dict, "/Matrix");
        for (int i = 0; ctm_aar && i < ctm_aar->num_elements; i++)
        {
            xobj->form->matrix[i] = ctm_aar->values[i]->val.number;
        }
        pdf_array_t* bbox_aar = pdf_dict_get_array(xobject_dict, "/BBox");
        for (int i = 0; bbox_aar && i < bbox_aar->num_elements; i++)
        {
            xobj->form->bbox[i] = bbox_aar->values[i]->val.number;
        }
        // obj->xobject = xobj;
        xobject->xobject = xobj;
        return xobj;
    }
    return NULL;
}