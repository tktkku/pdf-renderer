#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <assert.h>
#include <stdbool.h>

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

pdf_font_t* _load_type0_font(pdf_page_t* page, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    // pdf_dict_get_name(font_dict, "/Name"); // not used in PDF 1.7
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");
    if (font->encoding != NULL)
    {
        pdf_cmap_t* cmap = pdf_file_get_cmap(page->pdf, font->encoding);
        font->cmap = cmap;
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        // PDF Specification 1.7, 9.10.3 ToUnicode CMaps
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        pdf_parser_t* parser = pdf_parser_init(page->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
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
    }
    else
    {
        font->font_descriptor = pdf_dict_get_dict(font->descendant_font_dict, "/FontDescriptor");
    }
    if (font->font_descriptor != NULL)
    {
        // font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
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
pdf_font_t* _load_truetype_font(pdf_page_t* page, pdf_dict_t* font_dict)
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
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        pdf_parser_t* parser = pdf_parser_init(page->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
    }
    int font_descriptor_ref = pdf_dict_get_ref(font_dict, "/FontDescriptor");
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font_descriptor_ref);
        font->font_descriptor = obj->value->val.dict;
    }
    else
    {
        font->font_descriptor = pdf_dict_get_dict(font_dict, "/FontDescriptor");
    }
    if (font->font_descriptor != NULL)
    {
        // font->type = pdf_dict_get_name(font->font_descriptor, "/Type");
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
    return font;
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

    char* type = pdf_dict_get_name(font_dict, "/Type"); // Font
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
        return _load_truetype_font(page, font_dict);
    }
    else if (!strcmp(subtype, "/Type1"))
    {
        return NULL;
    }
    else if (!strcmp(subtype, "/Type0"))
    {
        return _load_type0_font(page, font_dict);
    }
    return NULL;
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