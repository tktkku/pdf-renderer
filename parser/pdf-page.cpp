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
    page->annots = NULL;
    return page;
}

int pdf_page_get_streams(pdf_page_t* page)
{
    if (page == NULL) return 0;

    return page->contents.size();
}
void pdf_page_get_ext_gstate(pdf_page_t* page, const char* name)
{
    if (page == NULL)
        return;
    if (page->resources == NULL)
        return;
    if (page->resources->ext_gstate == NULL)
        return;
    PdfDict* ext_gstate = (*page->resources->ext_gstate)[name].dict;
    if (ext_gstate == NULL)
    {
        int ref = (*page->resources->ext_gstate)[name].indirect;
        if (ref == -1)
        {
            return;
        }
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, ref);
        ext_gstate = obj->value->dict;
        if (ext_gstate == NULL)
        {
            return;
        }
    }
    auto& ext_dict = *ext_gstate;
    // const char* Type = pdf_dict_get_name(ext_gstate, "/Type");// shall be ExtGState
    // double LW = pdf_dict_get_number(ext_gstate, "/LW"); // line width
    // int LC = pdf_dict_get_number(ext_gstate, "/LC"); // line cap
    // int LJ = pdf_dict_get_number(ext_gstate, "/LJ"); // line join
    // double ML = pdf_dict_get_number(ext_gstate, "/ML"); // miter limit
    // PdfArray* D = pdf_dict_get_array(ext_gstate, "/D"); // dash pattern
    // const char* RI = pdf_dict_get_name(ext_gstate, "/RI");
    // int OP = pdf_dict_get_bool(ext_gstate, "/OP"); // whether to apply overprint
    // int op = pdf_dict_get_bool(ext_gstate, "/op");
    // int OPM = pdf_dict_get_number(ext_gstate, "/OPM");
    // PdfArray* Font = pdf_dict_get_array(ext_gstate, "/Font");
    // void* BG;
    // void* BG2;
    // void* UCR;
    // void* UCR2;
    // void* TR;
    // void* TR2;
    // void* HT;
    // double FL = pdf_dict_get_number(ext_gstate, "/FL");
    // double SM = pdf_dict_get_number(ext_gstate, "/SM");
    // int SA = pdf_dict_get_bool(ext_gstate, "/SA");
    // void* BM;
    // void* SMask;
    // double CA = pdf_dict_get_number(ext_gstate, "/CA");
    // double ca = pdf_dict_get_number(ext_gstate, "/ca");
    // int AIS = pdf_dict_get_bool(ext_gstate, "/AIS");
    // int TK = pdf_dict_get_bool(ext_gstate, "/TK");
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
pdf_font_t* _load_type0_font(pdf_page_t* page, PdfDict& font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    // pdf_dict_get_name(font_dict, "/Name"); // not used in PDF 1.7
    font->basefont = (char*)font_dict["/BaseFont"].name;
    font->encoding = (char*)font_dict["/Encoding"].name;
    if (font->encoding != NULL)
    {
        pdf_cmap_t* cmap = pdf_file_get_cmap(page->pdf, font->encoding);
        font->cmap = cmap;
    }
    int to_unicode_ref = -1;
    if (font_dict["/ToUnicode"].type == INDIRECT)
    {
        to_unicode_ref = font_dict["/ToUnicode"].indirect;
    }
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
        PdfParser cmap_parser(page->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = cmap_parser.buildCMap();
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
    }
    // CIDFonts
    font->descendant_font_dict = NULL;
   
    if (font_dict["/DescendantFonts"].type == ARRAY)
    {
        PdfArray* descendant_fonts_aar = font_dict["/DescendantFonts"].array;
        if ((*descendant_fonts_aar)[0]->type == DICT)
        {
            font->descendant_font_dict = (*descendant_fonts_aar)[0]->dict;
        }
        else if ((*descendant_fonts_aar)[0]->type == INDIRECT)
        {
            int descendant_fonts_ref = (*descendant_fonts_aar)[0]->indirect;
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendant_fonts_ref);
            font->descendant_font_dict = descendant_font_obj->value->dict;
        }
    }
    else
    {
        int descendantfonts_ref = -1;
        if (font_dict["/DescendantFonts"].type == INDIRECT)
        {
            descendantfonts_ref = font_dict["/DescendantFonts"].indirect;
        }
        if (descendantfonts_ref != -1)
        {
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendantfonts_ref);
            if (descendant_font_obj != NULL && descendant_font_obj->value->type == DICT)
            {
                font->descendant_font_dict = descendant_font_obj->value->dict;
            }
            else if (descendant_font_obj != NULL && descendant_font_obj->value->type == ARRAY)
            {
                PdfArray* descendant_fonts_aar = descendant_font_obj->value->array;
                if ((*descendant_fonts_aar)[0]->type == DICT)
                {
                    font->descendant_font_dict = (*descendant_fonts_aar)[0]->dict;
                }
                else if ((*descendant_fonts_aar)[0]->type == INDIRECT)
                {
                    int descendant_fonts_ref = (*descendant_fonts_aar)[0]->indirect;
                    pdf_obj_t* descendant_font_obj = pdf_file_get_obj(page->pdf, descendant_fonts_ref);
                    font->descendant_font_dict = descendant_font_obj->value->dict;
                }
            }
        }
    }
    if (font->descendant_font_dict == NULL)
    {
        pdf_font_free(font);
        return NULL;
    }

    auto& descendant_font_dict = *(font->descendant_font_dict);
    font->type = (char*)descendant_font_dict["/Type"].name; // Font
    font->subtype = (char*)descendant_font_dict["/Subtype"].name; // CIDFontType0 CIDFontType2
    // for CIDFontType0, it shall be the value of the CIDFontName entry
    // for CIDFontType2, 
    font->basefont = (char*)descendant_font_dict["/BaseFont"].name;
    font->cid_system_info_ref = -1;
    if (descendant_font_dict["/CIDSystemInfo"].type == INDIRECT)
    {
        font->cid_system_info_ref = descendant_font_dict["/CIDSystemInfo"].indirect;
    }
    if (font->cid_system_info_ref != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font->cid_system_info_ref);
        char* registry = (char*)(*obj->value->dict)["/Registry"].string;
        char* ordering = (char*)(*obj->value->dict)["/Ordering"].string;
        int supplement = (*obj->value->dict)["/Supplement"].number;
    }
    int font_descriptor_ref = -1;
    if (descendant_font_dict["/FontDescriptor"].type == INDIRECT)
    {
        font_descriptor_ref = descendant_font_dict["/FontDescriptor"].indirect;
    }
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font_descriptor_ref);
        font->font_descriptor = obj->value->dict;
        auto& font_descriptor_dict = *(obj->value->dict);
        font->type = (char*)font_descriptor_dict["/Type"].name;
        font->basefont = (char*)font_descriptor_dict["/FontName"].name;
        font->font_weight = font_descriptor_dict["/FontWeight"].number;
        font->flags = font_descriptor_dict["/Flags"].number;
        font->italic_angle = font_descriptor_dict["/ItalicAngle"].number;
        font->font_bbox = font_descriptor_dict["/FontBBox"].array;
        font->ascent = font_descriptor_dict["/Ascent"].number;
        font->descent = font_descriptor_dict["/Descent"].number;
        font->cap_height = font_descriptor_dict["/CapHeight"].number;
        font->stemv = font_descriptor_dict["/StemV"].number;
        font->cid_set_ref = font_descriptor_dict["/CIDSet"].indirect;

        font->fontfile1_ref = -1;
        if (font_descriptor_dict["/FontFile1"].type == INDIRECT)
        {
            font->fontfile1_ref = font_descriptor_dict["/FontFile1"].indirect;
        }
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
        font->fontfile2_ref = -1;
        if (font_descriptor_dict["/FontFile2"].type == INDIRECT)
        {
            font->fontfile2_ref = font_descriptor_dict["/FontFile2"].indirect;
        }
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
        font->fontfile3_ref = -1;
        if (font_descriptor_dict["/FontFile3"].type == INDIRECT)
        {
            font->fontfile3_ref = font_descriptor_dict["/FontFile3"].indirect;
        }
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
    
    if (descendant_font_dict["/DW"].type == NUMBER)
    {
        font->dw = descendant_font_dict["/DW"].number;
    }
    else
    {
        font->dw = 1000;
    }
    font->w_aar = descendant_font_dict["/W"].array;
    PdfArray* dw2_aar = descendant_font_dict["/DW2"].array;
    PdfArray* w2_aar = descendant_font_dict["/W2"].array;
    // shall be Identity
    
    if (descendant_font_dict["/CIDToGIDMap"].type == NAME)
    {
        font->cid_to_gid_map = (char*)descendant_font_dict["/CIDToGIDMap"].name;
    }
    else
    {
        font->cid_to_gid_map_ref = -1;
        if (descendant_font_dict["/CIDToGIDMap"].type == INDIRECT)
        {
            font->cid_to_gid_map_ref = descendant_font_dict["/CIDToGIDMap"].indirect;
        }
        if (font->cid_to_gid_map_ref != -1)
        {
            pdf_obj_t* cid_to_gid_obj = pdf_file_get_obj(page->pdf, font->cid_to_gid_map_ref);
            if (cid_to_gid_obj != NULL)
            {
                int size = 0;
                //pdf_stream_get_all(cid_to_gid_obj->stream, &font->cid_to_gid_map, &size);
            }
        }
    }

    return font;
}
pdf_font_t* _load_truetype_font(pdf_page_t* page, PdfDict& font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->type = (char*)font_dict["/Type"].name; // Font
    font->subtype = (char*)font_dict["/Subtype"].name;
    font->basefont = (char*)font_dict["/BaseFont"].name;
    font->first_char = font_dict["/FirstChar"].number;
    font->last_char = font_dict["/LastChar"].number;
    font->widths = font_dict["/Widths"].array;
    font->encoding = (char*)font_dict["/Encoding"].name;// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    
    if (font_dict["/ToUnicode"].type == INDIRECT)
    {
        int to_unicode_ref = font_dict["/ToUnicode"].indirect;
        // PDF Specification 1.7, 9.10.3 ToUnicode CMaps
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj->stream, &data, &len);
        pdf_buffer_t b1;
        b1.buffer = data;
        b1.buffer_size = len;
        b1.processed = 0;
        PdfParser cmap_parser(page->pdf, BUFFER_READER, &b1);

        pdf_cmap_t* cmap = cmap_parser.buildCMap();
        cmap->worldwide = false;
        font->to_unicode_map = cmap;
    }

    if (font_dict["/FontDescriptor"].type == INDIRECT)
    {
        int font_descriptor_ref = font_dict["/FontDescriptor"].indirect;
        pdf_obj_t* obj = pdf_file_get_obj(page->pdf, font_descriptor_ref);
        font->font_descriptor = obj->value->dict;
        auto& font_descriptor_dict = *(obj->value->dict);
        font->stemv = font_descriptor_dict["/StemV"].number;
        font->flags = font_descriptor_dict["/Flags"].number;
        font->italic_angle = font_descriptor_dict["/ItalicAngle"].number;
        font->font_bbox = font_descriptor_dict["/FontBBox"].array;
        font->ascent = font_descriptor_dict["/Ascent"].number;
        font->descent = font_descriptor_dict["/Descent"].number;
        font->cap_height = font_descriptor_dict["/CapHeight"].number;
    }
    
    return font;
}
pdf_font_t* pdf_page_get_font(pdf_page_t* page, const char* name)
{
    if (page == NULL || page->resources == NULL || page->resources->font_dict == NULL || name == NULL)
        return NULL;
    int ref = (*page->resources->font_dict)[name].indirect;
    if (ref == -1)
        return NULL;
    pdf_obj_t* font_obj = pdf_file_get_obj(page->pdf, ref);
    if (font_obj == NULL)
        return NULL;

    PdfDict& font_dict = (*font_obj->value->dict);

    char* type = (char*)font_dict["/Type"].name; // Font
    if (!type)
    {
        return NULL;
    }

    // Type0
    // Type1 MMType1
    // Type3
    char* subtype = (char*)font_dict["/Subtype"].name;
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
    if (index > page->contents.size() - 1) return NULL;

    pdf_obj_t* obj = page->contents[index];
    if (obj == NULL)
    {
        return NULL;
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

    // if (page->contents.size() > 0)
    // {
    //     for (auto* ptr : page->contents)
    //     {
    //         pdf_obj_free(ptr);
    //     }
    // }

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