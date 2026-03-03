#include "pdf.h"
#include "pdf-private.h"

#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>
#define STB_IMAGE_IMPLEMENTATION
#include "plutovg-stb-image.h"
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
        obj->font = NULL;
    }
    if (obj->font_data != NULL)
    {
        free(obj->font_data);
        obj->font_data = NULL;
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
pdf_font_descriptor_t* _load_font_descriptor(pdf_obj_t* obj, pdf_dict* font_dict)
{
    pdf_dict* font_descriptor_dict = NULL;
    if (font_dict->has("/FontDescriptor"))
    {
        if (font_dict->is_indirect("/FontDescriptor"))
        {
            int font_descriptor_ref = font_dict->get_indirect("/FontDescriptor");
            if (font_descriptor_ref != -1)
            {
                pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, font_descriptor_ref);
                if (obj1 == NULL || obj1->value->type != PDF_VALUE_DICT) 
                    font_descriptor_dict = NULL;
                else
                    font_descriptor_dict = obj1->value->val.dict;
            }
        }
        else if (font_dict->is_dict("/FontDescriptor"))
        {
            font_descriptor_dict = font_dict->get_dict("/FontDescriptor");
        }
    }
    else
    {
        return NULL;
    }

    pdf_font_descriptor_t* font_descriptor = (pdf_font_descriptor_t*)calloc(1, sizeof(pdf_font_descriptor_t));
    if (font_descriptor == NULL)
        return NULL;
    font_descriptor->fontName = (char*)font_descriptor_dict->get_name("/FontName");
    font_descriptor->fontFamily = (char*)font_descriptor_dict->get_name("/FontFamily");
    font_descriptor->fontStretch = (char*)font_descriptor_dict->get_name("/FontStretch");
    font_descriptor->fontWeight = font_descriptor_dict->get_number("/FontWeight");
    if (font_descriptor->fontWeight < 0)
        font_descriptor->fontWeight = 400;
    font_descriptor->flags = (uint32_t)font_descriptor_dict->get_number("/Flags");
    font_descriptor->fontBBox = font_descriptor_dict->get_array("/FontBBox");
    font_descriptor->italicAngle = font_descriptor_dict->get_number("/ItalicAngle");
    font_descriptor->ascent = font_descriptor_dict->get_number("/Ascent");
    font_descriptor->descent = font_descriptor_dict->get_number("/Descent");
    font_descriptor->leading = font_descriptor_dict->get_number("/Leading");
    font_descriptor->capHeight = font_descriptor_dict->get_number("/CapHeight");
    font_descriptor->xHeight = font_descriptor_dict->get_number("/XHeight");
    font_descriptor->stemV = font_descriptor_dict->get_number("/StemV");
    font_descriptor->stemH = font_descriptor_dict->get_number("/StemH");
    font_descriptor->avgWidth = font_descriptor_dict->get_number("/AvgWidth");
    font_descriptor->maxWidth = font_descriptor_dict->get_number("/MaxWidth");
    font_descriptor->missingWidth = font_descriptor_dict->get_number("/MissingWidth");
    font_descriptor->charSet = font_descriptor_dict->get_string("/CharSet");

    font_descriptor->cidSet = font_descriptor_dict->get_dict("/CIDSet");
    font_descriptor->style = font_descriptor_dict->get_dict("/Style");
    font_descriptor->lang = (char*)font_descriptor_dict->get_name("/Lang");
    font_descriptor->fd = font_descriptor_dict->get_dict("/FD");

    int fontfile_ref = font_descriptor_dict->get_indirect("/FontFile1");
    if (fontfile_ref == -1)
        fontfile_ref = font_descriptor_dict->get_indirect("/FontFile2");
    if (fontfile_ref == -1)
        fontfile_ref = font_descriptor_dict->get_indirect("/FontFile3");
    if (fontfile_ref != -1)
    {
        pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, fontfile_ref);
        if (fontfile_obj != NULL && fontfile_obj->stream != NULL)
        {
            pdf_dict* fontfile_dict = fontfile_obj->value->val.dict;
            char* subtype = (char*)fontfile_dict->get_name("/Subtype");
            if (fontfile_obj->font_data != NULL && fontfile_obj->font_data_len != 0)
            {
                font_descriptor->fontfile = fontfile_obj->font_data;
                font_descriptor->fontfile_len = fontfile_obj->font_data_len;
            }
            else
            {
                pdf_stream_get_all(fontfile_obj->stream, &font_descriptor->fontfile, &font_descriptor->fontfile_len);
                fontfile_obj->font_data = font_descriptor->fontfile;
                fontfile_obj->font_data_len = font_descriptor->fontfile_len;
                if (subtype && (!strcmp(subtype, "/CIDFontType0C") || !strcmp(subtype, "Type1C")))
                    pdf_cff_parse(font_descriptor);
            }
        }
    }

    return font_descriptor;
}
pdf_font_t* _load_cid_font(pdf_obj_t* obj, pdf_dict* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    
    char* subtype = (char*)font_dict->get_name("/Subtype"); // CIDFontType0 CIDFontType2
    if (strcmp(subtype, "/CIDFontType0") == 0)
    {
        font->subtype = FONT_SUBTYPE_CIDFONTTPYE0;
    }
    else
    {
        font->subtype = FONT_SUBTYPE_CIDFONTTPYE2;
    }
    font->cidfont = (pdf_font_cidfont_t*)calloc(1, sizeof(pdf_font_cidfont_t));
    if (font->cidfont == NULL)
    {
        pdf_font_free(font);
        return NULL;
    }
    // for CIDFontType0, it shall be the value of the CIDFontName entry
    // for CIDFontType2, 
    font->basefont = (char*)font_dict->get_name("/BaseFont");
    if (font_dict->has("/CIDSystemInfo"))
    {
        pdf_dict* cidsysteminfo_dict = NULL;
        if (font_dict->is_dict("/CIDSystemInfo"))
        {
            cidsysteminfo_dict = font_dict->get_dict("/CIDSystemInfo");
        }
        else if (font_dict->is_indirect("/CIDSystemInfo"))
        {
            int cid_system_info_ref = font_dict->get_indirect("/CIDSystemInfo");
            if (cid_system_info_ref != -1)
            {
                pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, cid_system_info_ref);
                if (obj1 != NULL && obj1->value->type == PDF_VALUE_DICT)
                    cidsysteminfo_dict = obj1->value->val.dict;
            }
        }
        if (cidsysteminfo_dict != NULL)
        {
            char* registry = cidsysteminfo_dict->get_string("/Registry");
            if (registry == NULL)
            {
                int ref = cidsysteminfo_dict->get_indirect("/Registry");
                pdf_obj_t* obj2 = pdf_file_get_obj(obj->pdf, ref);
                if (obj2 != NULL)
                {
                    registry = obj2->value->val.string;
                }
            }
            char* ordering = cidsysteminfo_dict->get_string("/Ordering");
            if (ordering == NULL)
            {
                int ref = cidsysteminfo_dict->get_indirect("/Ordering");
                pdf_obj_t* obj2 = pdf_file_get_obj(obj->pdf, ref);
                if (obj2 != NULL)
                {
                    ordering = obj2->value->val.string + 1;
                }
            }
            int supplement = cidsysteminfo_dict->get_number("/Supplement");
            font->cidfont->cid_system_info.registry = strdup(registry + 1);
            font->cidfont->cid_system_info.ordering = strdup(ordering + 1);
            font->cidfont->cid_system_info.supplement = supplement;
        }
    }

    font->cidfont->font_descriptor = _load_font_descriptor(obj, font_dict);

    font->cidfont->dw = font_dict->get_number("/DW");
    if (font->cidfont->dw == -1)
        font->cidfont->dw = 1000;
    font->cidfont->w_aar = font_dict->get_array("/W");
    font->cidfont->dw2_aar = font_dict->get_array("/DW2");
    font->cidfont->w2_aar = font_dict->get_array("/W2");
    // shall be Identity
    if (font_dict->has("/CIDToGIDMap"))
    {
        if (font_dict->is_name("/CIDToGIDMap"))
        {
            char* cid_to_gid_map = (char*)font_dict->get_name("/CIDToGIDMap");
            if (cid_to_gid_map != NULL)
            {
                if (!strcmp(cid_to_gid_map, "/Identity"))
                {
                    font->cidfont->cid_to_gid_map = pdf_file_get_cmap(obj->pdf, "Identity-H");
                }
                else
                {
                    font->cidfont->cid_to_gid_map = pdf_file_get_cmap(obj->pdf, cid_to_gid_map + 1);
                }
            }
        }
        else if (font_dict->is_indirect("/CIDToGIDMap"))
        {
            int cid_to_gid_map_ref = font_dict->get_indirect("/CIDToGIDMap");
            if (cid_to_gid_map_ref != -1)
            {
                pdf_obj_t* cid_to_gid_obj = pdf_file_get_obj(obj->pdf, cid_to_gid_map_ref);
                if (cid_to_gid_obj != NULL)
                {
                    int size = 0;
                    unsigned char* map = NULL;
                    pdf_stream_get_all(cid_to_gid_obj->stream, &map, &size);
                    if (map != NULL && size > 0)
                    {
                        input_t* input = NULL;
                        input_buffer(&input, (char*)map, size);
                        pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);
                        pdf_cmap* cmap = pdf_parser_build_cmap(parser);
                        cmap->isGlobal = false;
                        font->cidfont->cid_to_gid_map = cmap;
                        pdf_parser_free(parser);
                        input_close(input);
                        free(map);
                    }
                }
            }
        }
    }

    return font;
}

pdf_font_t* _load_type0_font(pdf_obj_t* obj, pdf_dict* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->subtype = FONT_SUBTYPE_TYPE0;
    font->basefont = (char*)font_dict->get_name("/BaseFont");
    font->type0 = (pdf_font_type0_t*)calloc(1, sizeof(pdf_font_type0_t));
    if (font->type0 == NULL)
    {
        pdf_font_free(font);
        return NULL;
    }
        
    char* encoding = (char*)font_dict->get_name("/Encoding");
    if (encoding != NULL)
    {
        font->type0->encoding = pdf_file_get_cmap(obj->pdf, encoding + 1);
    }

    if (font_dict->has("/ToUnicode"))
    {
        int to_unicode_ref = font_dict->get_indirect("/ToUnicode");
        // PDF Specification 1.7, 9.10.3 ToUnicode CMaps
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        if (obj1 == NULL || obj1->stream == NULL)
        {
            pdf_font_free(font);
            return NULL;
        }
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        if (data != NULL)
        {
            input_t* input = NULL;
            input_buffer(&input, (char*)data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);

            pdf_cmap* cmap = pdf_parser_build_cmap(parser);
            cmap->isGlobal = false;
            font->type0->to_unicode_map = cmap;
            pdf_parser_free(parser);
            input_close(input);
            free(origin);
        } 
    }
    // CIDFonts
    if (font_dict->has("/DescendantFonts"))
    {
        pdf_dict* descendant_font_dict = NULL;
        if (font_dict->is_array("/DescendantFonts"))
        {
            pdf_array* descendant_fonts_aar = font_dict->get_array("/DescendantFonts");
            if (descendant_fonts_aar != NULL && descendant_fonts_aar->get(0)->type == PDF_VALUE_DICT)
            {
                descendant_font_dict = descendant_fonts_aar->get(0)->val.dict;
            }
            else if (descendant_fonts_aar != NULL && descendant_fonts_aar->get(0)->type == PDF_VALUE_INDIRECT)
            {
                int descendant_fonts_ref = descendant_fonts_aar->get(0)->val.indirect;
                if (descendant_fonts_ref != -1)
                {
                    pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendant_fonts_ref);
                    if (descendant_font_obj != NULL && descendant_font_obj->value->type == PDF_VALUE_DICT)
                        descendant_font_dict = descendant_font_obj->value->val.dict;
                    else
                        descendant_font_dict = NULL;
                }
            }
        }
        else if (font_dict->is_indirect("/DescendantFonts"))
        {
            int descendantfonts_ref = font_dict->get_indirect("/DescendantFonts");
            if (descendantfonts_ref != -1)
            {
                pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendantfonts_ref);
                if (descendant_font_obj != NULL && descendant_font_obj->value->type == PDF_VALUE_DICT)
                {
                    descendant_font_dict = descendant_font_obj->value->val.dict;
                }
                else if (descendant_font_obj != NULL && descendant_font_obj->value->type == PDF_VALUE_ARRAY)
                {
                    pdf_array* descendant_fonts_aar = descendant_font_obj->value->val.array;
                    if (descendant_fonts_aar->get(0)->type == PDF_VALUE_DICT)
                    {
                        descendant_font_dict = descendant_fonts_aar->get(0)->val.dict;
                    }
                    else if (descendant_fonts_aar->get(0)->type == PDF_VALUE_INDIRECT)
                    {
                        int descendant_fonts_ref = descendant_fonts_aar->get(0)->val.indirect;
                        pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendant_fonts_ref);
                        descendant_font_dict = descendant_font_obj->value->val.dict;
                    }
                }
            }
        }
        if (descendant_font_dict != NULL)
        {
            font->type0->descendant = _load_cid_font(obj, descendant_font_dict);
        }
    }
    
    return font;
}
pdf_array* _load_differences(pdf_dict* font_dict)
{
    pdf_dict* encoding_dict = font_dict->get_dict("/Encoding");
    pdf_array* differences = NULL;
    if (encoding_dict != NULL)
    {
        pdf_array* arr = encoding_dict->get_array("/Differences");
        if (arr != NULL)
        {
            differences = new pdf_array;
            int cnt = 0;
            for (size_t i = 0; i < arr->size(); i++)
            {
                if (arr->get(i)->type == PDF_VALUE_NUMBER)
                {
                    pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    value->type = PDF_VALUE_NUMBER;
                    value->val.number = arr->get(i)->val.number;
                    differences->add(value);
                    cnt++;
                }
                else if (arr->get(i)->type == PDF_VALUE_NAME)
                {
                    if (cnt > 1 && differences->get(cnt - 1)->type == PDF_VALUE_NAME)
                    {
                        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                        value->type = PDF_VALUE_NUMBER;
                        value->val.number = differences->get(cnt - 2)->val.number + 1;
                        differences->add(value);
                        cnt++;
                    }
                    pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    value->type = PDF_VALUE_NAME;
                    value->val.name = strdup(arr->get(i)->val.name);
                    differences->add(value);
                    cnt++;
                }
            }
        }
    }
    return differences;
}
pdf_font_t* _load_type1_truetype_font(pdf_obj_t* obj, pdf_dict* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    char* subtype = (char*)font_dict->get_name("/Subtype");
    if (!strcmp(subtype, "/Type1"))
    {
        font->subtype = FONT_SUBTYPE_TYPE1;
    }
    else if (!strcmp(subtype, "/TrueType"))
    {
        font->subtype = FONT_SUBTYPE_TRUETYPE;
    }
    font->basefont = (char*)font_dict->get_name("/BaseFont");
    font->type1_truetype = (pdf_font_type1_t*)calloc(1, sizeof(pdf_font_type1_t));
    font->type1_truetype->name = (char*)font_dict->get_name("/Name");
    font->type1_truetype->first_char = font_dict->get_number("/FirstChar");
    font->type1_truetype->last_char = font_dict->get_number("/LastChar");
    font->type1_truetype->widths = font_dict->get_array("/Widths");
    char* encoding = (char*)font_dict->get_name("/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    if (encoding == NULL)
    {
        font->type1_truetype->differences = _load_differences(font_dict);
    }
    else
    {
        font->type1_truetype->encoding = encoding;
    }
    
    if (font_dict->has("/ToUnicode"))
    {
        int to_unicode_ref = font_dict->get_indirect("/ToUnicode");
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len = 0;
        pdf_stream_get_all(obj1->stream, &data, &len);
        if (data != NULL)
        {
            input_t* input = NULL;
            input_buffer(&input, (char*)data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);

            pdf_cmap* cmap = pdf_parser_build_cmap(parser);
            cmap->isGlobal = false;
            font->type1_truetype->to_unicode_map = cmap;
            input_close(input);
            free(origin);
        }
    }
    font->type1_truetype->font_descriptor = _load_font_descriptor(obj, font_dict);
    
    return font;
}
pdf_font_t* _load_type3_font(pdf_obj_t* obj, pdf_dict* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->subtype = FONT_SUBTYPE_TYPE3;
    font->basefont = (char*)font_dict->get_name("/BaseFont");
    font->type3 = (pdf_font_type3_t*)calloc(1, sizeof(pdf_font_type3_t));
    font->type3->first_char = font_dict->get_number("/FirstChar");
    font->type3->last_char = font_dict->get_number("/LastChar");
    if (font_dict->has("/Widths"))
    {
        if (font_dict->is_array("/Widths"))
        {
            font->type3->widths = font_dict->get_array("/Widths");
        }
        else if (font_dict->is_indirect("/Widths"))
        {
            int ref = font_dict->get_indirect("/Widths");
            if (ref != -1)
            {
                pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
                if (obj1 != NULL)
                {
                    font->type3->widths = obj1->value->val.array;
                }
            }
        }
    }

    font->type3->font_matrix = font_dict->get_array("/FontMatrix");
    font->type3->font_bbox = font_dict->get_array("/FontBBox");
    if (font_dict->has("/CharProcs"))
    {
        if (font_dict->is_dict("/CharProcs"))
        {
            font->type3->charProcs = font_dict->get_dict("/CharProcs");
        }
        else if (font_dict->is_indirect("/CharProcs"))
        {
            int ref = font_dict->get_indirect("/CharProcs");
            if (ref != -1)
            {
                pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
                if (obj1 != NULL && obj1->value->type == PDF_VALUE_DICT)
                {
                    font->type3->charProcs = obj1->value->val.dict;
                }
            }
        }
    }
  
    font->type3->encoding = (char*)font_dict->get_name("/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    if (font->type3->encoding == NULL)
    {
        font->type3->differences = _load_differences(font_dict);
    }
    
    if (font_dict->has("/ToUnicode"))
    {
        int to_unicode_ref = font_dict->get_indirect("/ToUnicode");
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        if (data != NULL)
        {
            input_t* input = NULL;
            input_buffer(&input, (char*)data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);
    
            pdf_cmap* cmap = pdf_parser_build_cmap(parser);
            cmap->isGlobal = false;
            font->type3->to_unicode_map = cmap;
            input_close(input);
            free(origin);
        }
    }
    font->type3->font_descriptor = _load_font_descriptor(obj, font_dict);

    return font;
}
void pdf_obj_get_colorspace(pdf_obj_t* obj, const char* name, char* value)
{
    if (obj == NULL || name == NULL || value == NULL || obj->resources.colorspace_dict == NULL)
    {
        return;
    }
    if (obj->resources.colorspace_dict->has(name))
    {
        if (obj->resources.colorspace_dict->is_name(name))
        {
            char* color_space = obj->resources.colorspace_dict->get_name(name);
            if (color_space != NULL)
            {
                strcpy(value, color_space);
                return;
            }
        }
        else if (obj->resources.colorspace_dict->is_indirect(name))
        {
            int ref = obj->resources.colorspace_dict->get_indirect(name);
            pdf_obj_t* color_space_obj = pdf_file_get_obj(obj->pdf, ref);
            if (color_space_obj != NULL)
            {
                if (color_space_obj->value->type == PDF_VALUE_NAME)
                {
                    strcpy(value, color_space_obj->value->val.name);
                }
            }
        }
    }
}
const static char* STANDARD_14_FONTS[][2] = {
    // "/Arial-BoldMT", "n019004l.pfb",
    // "/ArialMT", "n019003l.pfb",
    // "/Arial", "n019003l.pfb",
    // "/Arial-ItalicMT", "n019023l.pfb",
    // "/Arial-Italic", "n019023l.pfb"
    // "/CourierNewPSMT", "n022003l.pfb",

    {"/Courier", "n022003l.pfb"},
    {"/Courier-Bold", "n022004l.pfb"},
    {"/Courier-BoldOblique", "n022024l.pfb"},
    {"/Courier-Oblique", "n022023l.pfb"},
    
    {"/Symbol", "s050000l.pfb"},

    {"/Times-Bold", "p052004l.pfb"},
    {"/Times-BoldItalic", "p052024l.pfb"},
    {"/Times-Italic", "p052023l.pfb"},
    {"/Times-Roman", "p052003l.pfb"},
    // "/Times New Roman", "p052003l.pfb",
    // "/TimesNewRomanPSMT", "p052003l.pfb",
    // "/TimesNewRoman", "p052003l.pfb",
    // "/Times New Roman,Bold", "p052004l.pfb",
    // "/TimesNewRomanPS-BoldMT", "p052004l.pfb",
    // "/TimesNewRoman,Bold", "p052004l.pfb",
    // "/TimesNewRoman,Italic", "p052023l.pfb",
    // "/TimesNewRomanPS-ItalicMT", "p052023l.pfb",
    // "/TimesNewRomanPS-BoldItalicMT", "p052024l.pfb",
    // "/TimesNewRoman,BoldItalic", "p052024l.pfb",
    
    {"/Helvetica", "n019003l.pfb"},
    {"/Helvetica-Bold", "n019004l.pfb"},
    {"/Helvetica-BoldOblique", "n019024l.pfb"},
    {"/Helvetica-Oblique", "n019023l.pfb"},
    
    {"/ZapfDingbats", "d050000l.pfb"},
};

pdf_font_t* pdf_obj_get_font(pdf_obj_t* obj, const char* name)
{
    if (obj == NULL || name == NULL || obj->resources.font_dict == NULL) return NULL;
    if (!obj->resources.font_dict->has(name)) 
        return NULL;
    int ref = obj->resources.font_dict->get_indirect(name);
    pdf_obj_t* font_obj = pdf_file_get_obj(obj->pdf, ref);
    if (font_obj == NULL)
        return NULL;
    else if (font_obj->font != NULL)
    {
        return font_obj->font;
    }
    if (font_obj->value->type != PDF_VALUE_DICT) return NULL;
    pdf_dict* font_dict = font_obj->value->val.dict;
    if (!font_dict->has("/Type"))
    {
        return NULL;
    }
    char* type = (char*)font_dict->get_name("/Type"); // Font
    // Type0
    // Type1 MMType1
    // Type3
    if (!font_dict->has("/Subtype"))
    {
        return NULL;
    }
    char* subtype = (char*)font_dict->get_name("/Subtype");

    if (!strcmp(subtype, "/TrueType") || !strcmp(subtype, "/Type1"))
    {
        font_obj->font = _load_type1_truetype_font(obj, font_dict);
    }
    else if (!strcmp(subtype, "/Type3"))
    {
        font_obj->font = _load_type3_font(obj, font_dict);
    }
    else if (!strcmp(subtype, "/Type0"))
    {
        font_obj->font = _load_type0_font(obj, font_dict);
    }
    return font_obj->font;
}
void _get_smask(pdf_obj_t* smask_obj, pdf_image_t* img, int width, int height, pdf_array* color_space_aar)
{
    if (smask_obj == NULL || img == NULL)
        return;
    unsigned char* smask = NULL;
    int smask_len = 0;
    pdf_stream_get_all(smask_obj->stream, &smask, &smask_len);
    if (smask != NULL)
    {
        int tmp_len = sizeof(unsigned char) * width * 4 * height;
        unsigned char* tmp = (unsigned char*)malloc(tmp_len);
        int stride = img->data_len / height;
        if (color_space_aar != NULL && !strcmp(color_space_aar->get(0)->val.name, "/Indexed"))
        {
            int lookup_cnt = color_space_aar->get(2)->val.number;
            unsigned char* lookup = NULL;
            if (color_space_aar->get(3)->type == PDF_VALUE_INDIRECT)
            {
                int ref = color_space_aar->get(3)->val.indirect;
                pdf_obj_t* lookup_obj = pdf_file_get_obj(smask_obj->pdf, ref);
                pdf_stream_get_all(lookup_obj->stream, &lookup, &lookup_cnt);
            }
            else
            {
                lookup = (unsigned char*)(color_space_aar->get(3)->val.string + 1);
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
            if (color_space_aar->get(3)->type == PDF_VALUE_INDIRECT)
            {
                free(lookup);
            }
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
pdf_xobject_t* pdf_obj_get_xobject(pdf_obj_t* obj, const char* name)
{
    if (obj == NULL || obj->resources.xobject_dict == NULL || name == NULL)
        return NULL;
    // if (obj->xobject != NULL)
    //     return obj->xobject;
    // unsigned char* input = img_obj->stream;
    pdf_obj_t* xobject = NULL;
    pdf_dict* xobject_dict = NULL;
    if (obj->resources.xobject_dict->has(name))
    {
        if (obj->resources.xobject_dict->is_indirect(name))
        {
            int ref = obj->resources.xobject_dict->get_indirect(name);
            xobject = pdf_file_get_obj(obj->pdf, ref);
            if (xobject == NULL || xobject->stream == NULL) return NULL;
            else if (xobject->xobject != NULL)
            {
                return xobject->xobject;
            }
            xobject_dict = xobject->value->val.dict;
        }
        else if (obj->resources.xobject_dict->is_dict(name))
        {
            xobject_dict = obj->resources.xobject_dict->get_dict(name);
        }
    }

    if (xobject_dict == NULL) return NULL;
    const char* type = xobject_dict->get_name("/Type"); // XObject
    // if (strcmp(type, "/XObject") != 0)
    // {
    //     return NULL;
    // }
    const char* subtype = xobject_dict->get_name("/Subtype");
    const char* subtype2 = xobject_dict->get_name("/Subtype2");
    if (subtype == NULL) return NULL;
    if (strcmp(subtype, "/PS") == 0 || (subtype2 != NULL && !strcmp(subtype, "/Form") && !strcmp(subtype2, "/PS")))
    {
        // not used
    }
    else if (strcmp(subtype, "/Image") == 0)
    {
        // the value shall be one of 1 2 4 8 16
        // if ImageMask is true, this entry is optional, but if specified, its value shall be 1
        int bits_per_component = xobject_dict->get_number("/BitsPerComponent");
        const char* filter = NULL;
        if (xobject_dict->has("/Filter"))
        {
            if (xobject_dict->is_name("/Filter"))
            {
                filter = xobject_dict->get_name("/Filter");
                if (filter == NULL)
                {
                    return NULL;
                }
            }
            else if (xobject_dict->is_array("/Filter"))
            {
                pdf_array* filter_arr = xobject_dict->get_array("/Filter");
                if (filter_arr == NULL || filter_arr->size() == 0)
                {
                    return NULL;
                }
                filter = filter_arr->get(0)->val.name;
            }
        }
        else
        {
            return NULL;
        }
        
        int width = xobject_dict->get_number("/Width");
        int height = xobject_dict->get_number("/Height");
        int length = -1;
        if (xobject_dict->has("/Length"))
        {
            if (xobject_dict->is_number("/Length"))
            {
                length = xobject_dict->get_number("/Length");
            }
            else if (xobject_dict->is_indirect("/Length"))
            {
                int ref = xobject_dict->get_indirect("/Length");
                if (ref != -1)
                {
                    pdf_obj_t* l_obj = pdf_file_get_obj(obj->pdf, ref);
                    if (l_obj)
                    {
                        length = l_obj->value->val.number;
                    }
                }
            }
        }
        else
        {
            return NULL;
        }

        if (length <= 0) return NULL;
        const char* color_space = xobject_dict->get_name("/ColorSpace");
        //const char* name = xobject_dict->get_name("/Intent");
        //pdf_array* mask_arr = xobject_dict->get_array("/Mask");
        //pdf_array* decode_aar = xobject_dict->get_array("/Decode");
        //int interpolate = xobject_dict->get_bool("/Interpolate");
        //pdf_array* alter_aar = xobject_dict->get_array("/Alternates");
        int smask_ref = xobject_dict->get_indirect("/SMask");
        //int smask_in_data = xobject_dict->get_number("/SMaskInData");
        //const char* metadata = xobject_dict->get_name("/Metadata");
        //pdf_dict* oc_dict = xobject_dict->get_dict("/OC");
        pdf_array* color_space_aar = NULL;
        if (color_space == NULL)
        {
            color_space_aar = xobject_dict->get_array("/ColorSpace");
        }
        int imageMask = xobject_dict->get_boolean("/ImageMask");
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
            pdf_stream_get_all(xobject->stream, &img->data, &img->data_len);
            if (smask_ref != -1)
            {
                pdf_obj_t* smask_obj = pdf_file_get_obj(obj->pdf, smask_ref);
                if (smask_obj != NULL)
                {
                    _get_smask(smask_obj, img, width, height, color_space_aar);
                }
            }
            if (color_space_aar != NULL && !strcmp(color_space_aar->get(0)->val.name, "/Indexed"))
            {
                int stride = img->data_len / height;
                int tmp_len = sizeof(unsigned char) * width * 3 * height;
                unsigned char* tmp = (unsigned char*)malloc(tmp_len);
                int lookup_cnt = color_space_aar->get(2)->val.number;
                unsigned char* lookup = NULL;
                if (color_space_aar->get(3)->type == PDF_VALUE_INDIRECT)
                {
                    int ref = color_space_aar->get(3)->val.indirect;
                    pdf_obj_t* lookup_obj = pdf_file_get_obj(obj->pdf, ref);
                    pdf_stream_get_all(lookup_obj->stream, &lookup, &lookup_cnt);
                }
                else
                {
                    lookup = (unsigned char*)(color_space_aar->get(3)->val.string + 1);
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
                if (color_space_aar->get(3)->type == PDF_VALUE_INDIRECT)
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
            pdf_image_t* img = (pdf_image_t*)calloc(1, sizeof(pdf_image_t));
            img->width = width;
            img->height = height;
            img->bits_per_color = bits_per_component;
            img->data = (unsigned char*)calloc(length, sizeof(char));
            if (color_space != NULL)
            {
                strcpy(img->color_space, color_space);
            }
            input_seek(xobject->pdf->input, xobject->stream->stream_offset, SEEK_SET);
            int ret = input_read(xobject->pdf->input, img->data, length);
            if (ret != length)
            {
                free(img);
                return NULL;
            }
            img->data_len = length;
            if (smask_ref != -1)
            {
                pdf_obj_t* smask_obj = pdf_file_get_obj(obj->pdf, smask_ref);
                if (smask_obj != NULL)
                {
                    int channels = 3;
                    uint8_t* idata =  stbi_load_from_memory(img->data, img->data_len, &img->width, &img->height, &channels, 0);
                    if (idata != NULL)
                    {
                        pdf_image_t timg;
                        timg.data = idata;
                        timg.data_len = img->width * img->height * channels;
                        timg.width = img->width;
                        timg.height = img->height;
                        timg.bits_per_color = 8;
                        _get_smask(smask_obj, &timg, width, height, color_space_aar);
                        // stbi_image_free(idata);
                        img->data = timg.data;
                        img->data_len = timg.data_len;
                        img->width = timg.width;
                        img->height = timg.height;
                        img->bits_per_color = timg.bits_per_color;
                    }
                }
            }
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
        pdf_array* ctm_aar = xobject_dict->get_array("/Matrix");
        for (size_t i = 0; ctm_aar && i < ctm_aar->size(); i++)
        {
            xobj->form->matrix[i] = ctm_aar->get(i)->val.number;
        }
        pdf_array* bbox_aar = xobject_dict->get_array("/BBox");
        for (size_t i = 0; bbox_aar && i < bbox_aar->size(); i++)
        {
            xobj->form->bbox[i] = bbox_aar->get(i)->val.number;
        }
        // obj->xobject = xobj;
        xobject->xobject = xobj;
        return xobj;
    }
    return NULL;
}