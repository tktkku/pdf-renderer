#include "pdf-private.h"
#include "pdf.h"
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
pdf_font_descriptor_t* _load_font_descriptor(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    int font_descriptor_ref = pdf_dict_get_ref(font_dict, "/FontDescriptor");
    pdf_dict_t* font_descriptor_dict = NULL;
    if (font_descriptor_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, font_descriptor_ref);
        if (obj1 == NULL || obj1->value->type != DICT) 
            font_descriptor_dict = NULL;
        else
           font_descriptor_dict = obj1->value->val.dict;
    }
    else
    {
        font_descriptor_dict = pdf_dict_get_dict(font_dict, "/FontDescriptor");
    }
    if (font_descriptor_dict == NULL)
        return NULL;
    pdf_font_descriptor_t* font_descriptor = (pdf_font_descriptor_t*)calloc(1, sizeof(pdf_font_descriptor_t));
    if (font_descriptor == NULL)
        return NULL;
    font_descriptor->fontName = (char*)pdf_dict_get_name(font_descriptor_dict, "/FontName");
    font_descriptor->fontFamily = (char*)pdf_dict_get_name(font_descriptor_dict, "/FontFamily");
    font_descriptor->fontStretch = (char*)pdf_dict_get_name(font_descriptor_dict, "/FontStyle");
    font_descriptor->fontWeight = pdf_dict_get_number(font_descriptor_dict, "/FontWeight");
    font_descriptor->flags = (uint32_t)pdf_dict_get_number(font_descriptor_dict, "/Flags");
    font_descriptor->fontBBox = pdf_dict_get_array(font_descriptor_dict, "/FontBBox");
    font_descriptor->italicAngle = pdf_dict_get_number(font_descriptor_dict, "/ItalicAngle");
    font_descriptor->ascent = pdf_dict_get_number(font_descriptor_dict, "/Ascent");
    font_descriptor->descent = pdf_dict_get_number(font_descriptor_dict, "/Descent");
    font_descriptor->leading = pdf_dict_get_number(font_descriptor_dict, "/Leading");
    font_descriptor->capHeight = pdf_dict_get_number(font_descriptor_dict, "/CapHeight");
    font_descriptor->xHeight = pdf_dict_get_number(font_descriptor_dict, "/XHeight");
    font_descriptor->stemV = pdf_dict_get_number(font_descriptor_dict, "/StemV");
    font_descriptor->stemH = pdf_dict_get_number(font_descriptor_dict, "/StemH");
    font_descriptor->avgWidth = pdf_dict_get_number(font_descriptor_dict, "/AvgWidth");
    font_descriptor->maxWidth = pdf_dict_get_number(font_descriptor_dict, "/MaxWidth");
    font_descriptor->missingWidth = pdf_dict_get_number(font_descriptor_dict, "/MissingWidth");
    font_descriptor->charSet = pdf_dict_get_string(font_descriptor_dict, "/CharSet");

    font_descriptor->cidSet = pdf_dict_get_dict(font_descriptor_dict, "/CIDSet");
    font_descriptor->style = pdf_dict_get_dict(font_descriptor_dict, "/Style");
    font_descriptor->lang = (char*)pdf_dict_get_name(font_descriptor_dict, "/Lang");
    font_descriptor->fd = pdf_dict_get_dict(font_descriptor_dict, "/FD");

    int fontfile_ref = pdf_dict_get_ref(font_descriptor_dict, "/FontFile1");
    if (fontfile_ref == -1)
        fontfile_ref = pdf_dict_get_ref(font_descriptor_dict, "/FontFile2");
    if (fontfile_ref == -1)
        fontfile_ref = pdf_dict_get_ref(font_descriptor_dict, "/FontFile3");
    if (fontfile_ref != -1)
    {
        pdf_obj_t* fontfile_obj = pdf_file_get_obj(obj->pdf, fontfile_ref);
        if (fontfile_obj != NULL && fontfile_obj->stream != NULL)
        {
            pdf_dict_t* fontfile_dict = fontfile_obj->value->val.dict;
            char* subtype = (char*)pdf_dict_get_name(fontfile_dict, "/Subtype");
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
                if (subtype != NULL)
                    pdf_cff_parse(font_descriptor);
            }
        }
    }

    return font_descriptor;
}
pdf_font_t* _load_cid_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    
    char* subtype = (char*)pdf_dict_get_name(font_dict, "/Subtype"); // CIDFontType0 CIDFontType2
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
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    pdf_dict_t* cidsysteminfo_dict = pdf_dict_get_dict(font_dict, "/CIDSystemInfo");
    if (cidsysteminfo_dict == NULL)
    {
        int cid_system_info_ref = pdf_dict_get_ref(font_dict, "/CIDSystemInfo");
        if (cid_system_info_ref != -1)
        {
            pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, cid_system_info_ref);
            cidsysteminfo_dict = obj1->value->val.dict;
        }
    }

    if (cidsysteminfo_dict != NULL)
    {
        char* registry = pdf_dict_get_string(cidsysteminfo_dict, "/Registry");
        if (registry == NULL)
        {
            int ref = pdf_dict_get_ref(cidsysteminfo_dict, "/Registry");
            pdf_obj_t* obj2 = pdf_file_get_obj(obj->pdf, ref);
            if (obj2 != NULL)
            {
                registry = obj2->value->val.string;
            }
        }
        char* ordering = pdf_dict_get_string(cidsysteminfo_dict, "/Ordering");
        if (ordering == NULL)
        {
            int ref = pdf_dict_get_ref(cidsysteminfo_dict, "/Ordering");
            pdf_obj_t* obj2 = pdf_file_get_obj(obj->pdf, ref);
            if (obj2 != NULL)
            {
                ordering = obj2->value->val.string + 1;
            }
        }
        int supplement = pdf_dict_get_number(cidsysteminfo_dict, "/Supplement");
        font->cidfont->cid_system_info.registry = strdup(registry + 1);
        font->cidfont->cid_system_info.ordering = strdup(ordering + 1);
        font->cidfont->cid_system_info.supplement = supplement;


    }


    font->cidfont->font_descriptor = _load_font_descriptor(obj, font_dict);

    font->cidfont->dw = pdf_dict_get_number(font_dict, "/DW");
    if (font->cidfont->dw == -1)
        font->cidfont->dw = 1000;
    font->cidfont->w_aar = pdf_dict_get_array(font_dict, "/W");
    font->cidfont->dw2_aar = pdf_dict_get_array(font_dict, "/DW2");
    font->cidfont->w2_aar = pdf_dict_get_array(font_dict, "/W2");
    // shall be Identity
    char* cid_to_gid_map = (char*)pdf_dict_get_name(font_dict, "/CIDToGIDMap");
    if (cid_to_gid_map != NULL)
    {
        font->cidfont->cid_to_gid_map = pdf_cmap_find(cid_to_gid_map + 1);
    }
    else
    {   
        int cid_to_gid_map_ref = pdf_dict_get_ref(font_dict, "/CIDToGIDMap");
        if (cid_to_gid_map_ref != -1)
        {
            pdf_obj_t* cid_to_gid_obj = pdf_file_get_obj(obj->pdf, cid_to_gid_map_ref);
            if (cid_to_gid_obj != NULL)
            {
                int size = 0;
                unsigned char* map = NULL;
                pdf_stream_get_all(cid_to_gid_obj->stream, &map, &size);
                // TODO: parse map
            }
        }
    }
    if (font->cidfont->cid_to_gid_map == NULL && font->subtype == FONT_SUBTYPE_CIDFONTTPYE2)
    {
        font->cidfont->cid_to_gid_map = pdf_cmap_find("Identity-H");
    }
    return font;
}

pdf_font_t* _load_type0_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->subtype = FONT_SUBTYPE_TYPE0;
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->type0 = (pdf_font_type0_t*)calloc(1, sizeof(pdf_font_type0_t));
    if (font->type0 == NULL)
    {
        pdf_font_free(font);
        return NULL;
    }
        
    char* encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");
    if (encoding != NULL)
    {
        font->type0->encoding = pdf_cmap_find(encoding + 1);
    }

    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
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
            input_buffer(&input, data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);

            pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
            cmap->worldwide = false;
            font->type0->to_unicode_map = cmap;
            pdf_parser_free(parser);
            input_close(input);
            free(origin);
        } 
    }
    // CIDFonts
    pdf_dict_t* descendant_font_dict = NULL;
    pdf_array_t* descendant_fonts_aar = pdf_dict_get_array(font_dict, "/DescendantFonts");
    if (descendant_fonts_aar != NULL && descendant_fonts_aar->values[0]->type == DICT)
    {
        descendant_font_dict = descendant_fonts_aar->values[0]->val.dict;
    }
    else if (descendant_fonts_aar != NULL && descendant_fonts_aar->values[0]->type == INDIRECT)
    {
        int descendant_fonts_ref = descendant_fonts_aar->values[0]->val.indirect;
        if (descendant_fonts_ref != -1)
        {
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendant_fonts_ref);
            if (descendant_font_obj != NULL && descendant_font_obj->value->type == DICT)
                descendant_font_dict = descendant_font_obj->value->val.dict;
            else
                descendant_font_dict = NULL;
        }
    }
    else
    {
        int descendantfonts_ref = pdf_dict_get_ref(font_dict, "/DescendantFonts");
        if (descendantfonts_ref != -1)
        {
            pdf_obj_t* descendant_font_obj = pdf_file_get_obj(obj->pdf, descendantfonts_ref);
            if (descendant_font_obj != NULL && descendant_font_obj->value->type == DICT)
            {
                descendant_font_dict = descendant_font_obj->value->val.dict;
            }
            else if (descendant_font_obj != NULL && descendant_font_obj->value->type == ARRAY)
            {
                pdf_array_t* descendant_fonts_aar = descendant_font_obj->value->val.array;
                if (descendant_fonts_aar->values[0]->type == DICT)
                {
                    descendant_font_dict = descendant_fonts_aar->values[0]->val.dict;
                }
                else if (descendant_fonts_aar->values[0]->type == INDIRECT)
                {
                    int descendant_fonts_ref = descendant_fonts_aar->values[0]->val.indirect;
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

    return font;
}
pdf_array_t* _load_differences(pdf_dict_t* font_dict)
{
    pdf_dict_t* encoding_dict = pdf_dict_get_dict(font_dict, "/Encoding");
    pdf_array_t* differences = NULL;
    if (encoding_dict != NULL)
    {
        pdf_array_t* arr = pdf_dict_get_array(encoding_dict, "/Differences");
        if (arr != NULL)
        {
            differences = pdf_array_init();
            differences->num_elements = arr->num_elements * 2;
            differences->values = (pdf_array_element_value_t**)malloc(differences->num_elements * sizeof(pdf_array_element_value_t*));
            int cnt = 0;
            for (int i = 0; i < arr->num_elements; i++)
            {
                if (arr->values[i]->type == NUMBER)
                {
                    differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    differences->values[cnt]->type = NUMBER;
                    differences->values[cnt]->val.number = arr->values[i]->val.number;
                    cnt++;
                }
                else if (arr->values[i]->type == NAME)
                {
                    if (cnt > 1 && differences->values[cnt - 1]->type == NAME)
                    {
                        differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                        differences->values[cnt]->type = NUMBER;
                        differences->values[cnt]->val.number = differences->values[cnt - 2]->val.number + 1;
                        cnt++;
                    }
                    differences->values[cnt] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    differences->values[cnt]->type = NAME;
                    differences->values[cnt]->val.name = strdup(arr->values[i]->val.name);
                    cnt++;
                }
            }
            pdf_array_element_value_t** tmp = (pdf_array_element_value_t**)realloc(differences->values, cnt * sizeof(pdf_array_element_value_t*));
            if (tmp != NULL)
            {
                differences->values = tmp;
                differences->num_elements = cnt;
            }
            else
            {
                pdf_array_free(differences);
                differences = NULL;
            }
        }
    }
    return differences;
}
pdf_font_t* _load_type1_truetype_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    char* subtype = (char*)pdf_dict_get_name(font_dict, "/Subtype");
    if (!strcmp(subtype, "/Type1"))
    {
        font->subtype = FONT_SUBTYPE_TYPE1;
    }
    else if (!strcmp(subtype, "/TrueType"))
    {
        font->subtype = FONT_SUBTYPE_TRUETYPE;
    }
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->type1_truetype = (pdf_font_type1_t*)calloc(1, sizeof(pdf_font_type1_t));
    font->type1_truetype->name = (char*)pdf_dict_get_name(font_dict, "/Name");
    font->type1_truetype->first_char = pdf_dict_get_number(font_dict, "/FirstChar");
    font->type1_truetype->last_char = pdf_dict_get_number(font_dict, "/LastChar");
    font->type1_truetype->widths = pdf_dict_get_array(font_dict, "/Widths");
    char* encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    if (encoding == NULL)
    {
        font->type1_truetype->differences = _load_differences(font_dict);
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len = 0;
        pdf_stream_get_all(obj1->stream, &data, &len);
        if (data != NULL)
        {
            input_t* input = NULL;
            input_buffer(&input, data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);

            pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
            cmap->worldwide = false;
            font->type1_truetype->to_unicode_map = cmap;
            input_close(input);
            free(origin);
        }
    }
    font->type1_truetype->font_descriptor = _load_font_descriptor(obj, font_dict);
    
    return font;
}
pdf_font_t* _load_type3_font(pdf_obj_t* obj, pdf_dict_t* font_dict)
{
    pdf_font_t* font = pdf_font_init();
    if (font == NULL)
        return NULL;
    font->subtype = FONT_SUBTYPE_TYPE3;
    font->basefont = (char*)pdf_dict_get_name(font_dict, "/BaseFont");
    font->type3 = (pdf_font_type3_t*)calloc(1, sizeof(pdf_font_type3_t));
    font->type3->first_char = pdf_dict_get_number(font_dict, "/FirstChar");
    font->type3->last_char = pdf_dict_get_number(font_dict, "/LastChar");
    font->type3->widths = pdf_dict_get_array(font_dict, "/Widths");
    if (font->type3->widths == NULL)
    {
        int ref = pdf_dict_get_ref(font_dict, "/Widths");
        if (ref != -1)
        {
            pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
            if (obj1 != NULL)
            {
                font->type3->widths = obj1->value->val.array;
            }
        }
    }
    font->type3->font_matrix = pdf_dict_get_array(font_dict, "/FontMatrix");
    font->type3->font_bbox = pdf_dict_get_array(font_dict, "/FontBBox");
    font->type3->charProcs = pdf_dict_get_dict(font_dict, "/CharProcs");
    if (font->type3->charProcs == NULL)
    {
        int ref = pdf_dict_get_ref(font_dict, "/CharProcs");
        if (ref != -1)
        {
            pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, ref);
            if (obj1 != NULL)
            {
                font->type3->charProcs = obj1->value->val.dict;
            }
        }
    }   
    font->type3->encoding = (char*)pdf_dict_get_name(font_dict, "/Encoding");// MacRomanEncoding MacExpertEncoding WinAnsiEncoding
    if (font->type3->encoding == NULL)
    {
        font->type3->differences = _load_differences(font_dict);
    }
    int to_unicode_ref = pdf_dict_get_ref(font_dict, "/ToUnicode");
    if (to_unicode_ref != -1)
    {
        pdf_obj_t* obj1 = pdf_file_get_obj(obj->pdf, to_unicode_ref);
        unsigned char* data = NULL;
        int len;
        pdf_stream_get_all(obj1->stream, &data, &len);
        if (data != NULL)
        {
            input_t* input = NULL;
            input_buffer(&input, data, len);
            unsigned char* origin = data;
            pdf_parser_t* parser = pdf_parser_init(obj->pdf, input);
    
            pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
            cmap->worldwide = false;
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
    if (font_obj->value->type != DICT) return NULL;
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
        if (xobject == NULL || xobject->stream == NULL) return NULL;
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
    if (subtype == NULL) return NULL;
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
                            if (color_space_aar->values[3]->type == INDIRECT)
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
                if (color_space_aar->values[3]->type == INDIRECT)
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
            input_seek(xobject->pdf->input, xobject->stream->stream_offset, SEEK_SET);
            int ret = input_read(xobject->pdf->input, img->data, length);
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