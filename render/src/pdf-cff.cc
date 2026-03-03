#include "pdf.h"
#include "pdf-private.h"

#include "pdf-cff.h"
#include <string.h>
pdf_array* _parse_cff_index(unsigned char** p)
{
    if (p == NULL || *p == NULL) return NULL;
    uint16_t count = (*p)[0] << 8 | (*p)[1];(*p) += 2;
    uint8_t offSize = (*p)[0]; (*p) += 1;
    uint32_t* offsets = (uint32_t*)malloc(sizeof(uint32_t)* (count + 1));
    uint8_t shifts[] = {0, 8, 16, 24};
    for (int i = 0; i < count + 1; i++)
    {
        offsets[i] = 0;
        for (int j = 0; j < offSize; j++)
        {
            offsets[i] |= ((*p)[j] << shifts[offSize - 1 - j]);
        }
        (*p) += offSize;
    }
    pdf_array* arr = new pdf_array;
    for (int i = 1; i < count + 1; i++)
    {
        uint32_t len = offsets[i] - offsets[i - 1];
        pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        v->type = PDF_VALUE_STRING;
        v->value_len = len;
        v->val.string = (char*)malloc(len + 1);
        memcpy(v->val.string, (*p), len);
        v->val.string[len] = '\0';
        arr->add(v);
        (*p) += len;
    }
    
    free(offsets);
    return arr;
}

const char* _parse_cff_sid_to_string(uint16_t sid, pdf_array* string_index)
{
    if (sid >= 0 && sid <= CFF_NUM_STANDARD_STRINGS)
    {
        return CFF_STANDARD_STRINGS[sid];
    }
    else
    {
        return string_index->get(sid - CFF_NUM_STANDARD_STRINGS - 1)->val.string;
    }
}
void _parse_cff_try_skip_value(unsigned char** pp, pdf_deque* deque)
{
    unsigned char* p = *pp;
    double v = 0;
    uint8_t b0 = p[0];
    if (b0 == 28)
    {
        p++;
        uint8_t b1 = p[0];
        uint8_t b2 = p[1];
        p += 2;
        v = b1 << 8 | b2;
        deque->push_front(&v, sizeof(double)); 
    }
    else if (b0 == 29)
    {
        p++;
        uint8_t b1 = p[0];
        uint8_t b2 = p[1];
        uint8_t b3 = p[2];
        uint8_t b4 = p[3];
        p += 4;
        v = b1 << 24 | b2 << 16 | b3 << 8 | b4;
        deque->push_front(&v, sizeof(double));
    }
    else if (b0 == 30)
    {
        p++;
        unsigned char buf[66] = {0};
        char h,l;
        int cnt = 0;
        do
        {
            h = (p[0] >> 4) & 0x0F;
            l = (p[0] & 0x0F);
            p++;
            if (h >=0 && h <= 9) buf[cnt++] = '0' + h;
            else if (h == 0xA) buf[cnt++] = '.';
            else if (h == 0xB) buf[cnt++] = 'E';
            else if (h == 0xC)
            {
                buf[cnt++] = 'E';
                buf[cnt++] = '-';
            }
            else if (h == 0xD);
            else if (h == 0xE) buf[cnt++] = '-';
            else if (h == 0xF) break;

            if (l >=0 && l <= 9) buf[cnt++] = '0' + l;
            else if (l == 0xA) buf[cnt++] = '.';
            else if (l == 0xB) buf[cnt++] = 'E';
            else if (l == 0xC)
            {
                buf[cnt++] = 'E';
                buf[cnt++] = '-';
            }
            else if (l == 0xD);
            else if (l == 0xE) buf[cnt++] = '-';
            else if (l == 0xF) break;
        } while(h != 0xF && l != 0xF);
        v = strtod((char*)buf, NULL);
        deque->push_front(&v, sizeof(double));
    }
    else if (b0 >= 32 && b0 <= 246)
    {
        p++;
        v = b0 - 139;
        deque->push_front(&v, sizeof(double));
    }
    else if (b0 >= 247 && b0 <= 250)
    {
        p++;
        uint8_t b1 = p[0];
        p++;
        v = (b0 - 247) * 256 + b1 + 108;
        deque->push_front(&v, sizeof(double));
    }
    else if (b0 >= 251 && b0 <= 254)
    {
        p++;
        uint8_t b1 = p[0];
        p++;
        v = -(b0 - 251) * 256 - b1 - 108;
        deque->push_front(&v, sizeof(double));
    }
    *pp = p;
}

void _parse_cff_dict_data(unsigned char* font_data, 
    unsigned char* p, int len, 
    pdf_array* string_index,
    pdf_dict* ret_dict)
{
    pdf_deque* deque = new pdf_deque();
    unsigned char* end = p + len;
    while (p < end)
    {
        _parse_cff_try_skip_value(&p, deque);
        
        uint8_t operator1 = p[0];
        switch (operator1)
        {
            case 0:// version SID
            case 1:// Notice SID
            case 2:// FullName SID
            case 3:// FamilyName SID
            case 4:// Weight SID
            {
                const char* names[] = {"version", "Notice", "FullName", "FamilyName", "Weight"};
                auto data = deque->pop_back();
                uint32_t sid = *((double*)data->data());
                ret_dict->add(names[operator1], PDF_VALUE_STRING, strdup(_parse_cff_sid_to_string(sid, string_index))); 
                p++;
                break;
            }
            case 5: // FontBBox
            {
                pdf_array* a = new pdf_array;
                for (int i = 0; i < 4; i++)
                {
                    auto data = deque->pop_back();
                    pdf_value_t* arr_v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    arr_v->type = PDF_VALUE_NUMBER;
                    arr_v->val.number = *((double*)data->data());
                    a->add(arr_v);
                }
                ret_dict->add("FontBBox", PDF_VALUE_ARRAY, a);
                p++;
                break;
            }
            case 6://BlueValues
            case 7://OtherBlues
            case 8://FamilyBlues
            case 9://FamilyOtherBlues
            {
                deque->clear();
                p++;
                break;
            }
            case 10://StdHW
            case 11://StdVW
            {
                const char* names[] = {"StdHW", "StdVW"};
                auto data = deque->pop_back();
                ret_dict->add(names[operator1 - 10], PDF_VALUE_NUMBER, data->data());
                p++;
                break;
            }
            case 14: // XUID
            {
                deque->clear();
                p++;
                break;
            }
            case 13: // UniqueID
            case 15: // charset
            case 16: // Encoding
            case 17: // CharStrings
            {
                const char* names[] = {"UniqueID", "XUID", "charset", "Encoding", "CharStrings"};
                auto data = deque->pop_back();
                ret_dict->add(names[operator1 - 13], PDF_VALUE_NUMBER, data->data());
                p++;
                break;
            }
            case 18: // Private
            {
                auto data = deque->pop_back();
                uint32_t size = *((double*)data->data());
                auto data2 = deque->pop_back();
                uint32_t offset = *((double*)data2->data());

                _parse_cff_dict_data(font_data, font_data + offset, size, string_index, ret_dict);
                p++;
                break;
            }
            case 19:// Subrs(local)
            case 20:// defaultWidthX
            case 21:// nominalWidthX
            {
                const char* names[] = {"Subrs", "defaultWidthX", "nominalWidthX"};
                auto data = deque->pop_back();
                ret_dict->add(names[operator1 - 19], PDF_VALUE_NUMBER, data->data());
                p++;
                break;
            }
            case 12:
            {
                uint8_t operator2 = p[1];
                switch (operator2)
                {
                case 0: // Copyright
                {
                    p += 2;
                    deque->clear();
                    break;
                }
                case 1: // isFixedPitch
                {
                    p += 2;
                    deque->clear();
                    break;
                }
                case 7: // FontMatrix
                {
                    pdf_array* fontmatrix = new pdf_array;
                    for (int i = 0; i < 6; i++)
                    {
                        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                        value->type = PDF_VALUE_NUMBER;
                        auto data = deque->pop_back();
                        value->val.number = *((double*)data->data());
                        fontmatrix->add(value);
                    }
                    ret_dict->add("FontMatrix", PDF_VALUE_ARRAY, fontmatrix);
                    p += 2;
                    break;
                }
                case 2: // ItalicAngle
                case 3: // UnderlinePosition
                case 4: // UnderlineThickness
                case 5: // PaintType
                case 6: // CharstringType
                case 8: // StrokeWidth
                {
                    const char* names[] = {"ItalicAngle", "UnderlinePosition", "UnderlineThickness", "PaintType",
                    "CharstringType", "FontMatrix", "StrokeWidth"};
                    auto data = deque->pop_back();
                    ret_dict->add(names[operator2 - 2], PDF_VALUE_NUMBER, data->data());
                    p += 2;
                    break;
                }
                case 9://BlueScale
                case 10://BlueShift
                case 11://BlueFuzz
                {
                    deque->clear();
                    p += 2;
                    break;
                }
                case 12://StemSnapH
                case 13://StemSnapV
                {
                    deque->clear();
                    p += 2;
                    break;
                }
                case 14: // ForceBold
                {
                    deque->clear();
                    p += 2;
                    break;
                }
                case 17:// LangiageGroup
                case 18:// ExpansionFactor
                case 19://initialRandomSeed
                case 20: // SyntheticBase
                {
                    deque->clear();
                    p += 2;
                    break;
                }
                case 21: // PostScript
                case 22: // BaseFontName
                case 23: // BaseFontBlend
                {
                    p += 2;
                    deque->clear();
                    break;
                }
                case 30: // ROS
                {
                    auto data = deque->pop_back();
                    uint16_t sid1 = *((double*)data->data());
                    auto data2 = deque->pop_back();
                    uint16_t sid2 = *((double*)data2->data());
                    auto data3 = deque->pop_back();
                    double number = *((double*)data3->data());

                    pdf_array* array = new pdf_array();

                    pdf_value_t* arr_v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    arr_v->type = PDF_VALUE_STRING;
                    const char* s1 = _parse_cff_sid_to_string(sid1, string_index);
                    arr_v->val.string = strdup(s1);
                    array->add(arr_v);

                    pdf_value_t* arr_v2 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    arr_v2->type = PDF_VALUE_STRING;
                    const char* s2 = _parse_cff_sid_to_string(sid2, string_index);
                    arr_v2->val.string = strdup(s2);
                    array->add(arr_v2);

                    pdf_value_t* arr_v3 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    arr_v3->type = PDF_VALUE_NUMBER;
                    arr_v3->val.number = number;
                    array->add(arr_v3);

                    ret_dict->add("ROS", PDF_VALUE_ARRAY, array);

                    p += 2;
                    break;
                }
                case 31: // CIDFontVersion
                case 32: // CIDFontRevision
                case 33: // CIDFontType
                case 34: // CIDCount
                case 35: // UIDBase
                case 36: // FDArray
                case 37: // FDSelect
                {
                    const char* names[] = {"CIDFontVersion", "CIDFontRevision", "CIDFontType",
                    "CIDCount", "UIDBase", "FDArray", "FDSelect"};
                    auto data = deque->pop_back();
                    ret_dict->add(names[operator2 - 31], PDF_VALUE_NUMBER, data->data());
                    p += 2;

                    break;
                }
                case 38: // FontName
                {
                    p += 2;
                    deque->clear();
                    break;
                }
                default:
                    break;
                }
            }
                break;
            default:
                break;
        }
    }
    delete deque;
    //return ret_dict;
}
pdf_array* _parse_cff_charset(unsigned char* p, int count, pdf_array* string_index, bool isCIDFont)
{
    uint8_t format = p[0];p++;
    pdf_array* charset_arr = NULL;
    charset_arr = new pdf_array();
    if (format == 0)
    {
        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        if (isCIDFont)
        {
            value->type = PDF_VALUE_NUMBER;
            value->val.number = 0;
            charset_arr->add(value);
            for (int i = 1; i < count; i++)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                v->type = PDF_VALUE_NUMBER;
                v->val.number = sid;
                charset_arr->add(v);
            }
        }
        else
        {
            value->type = PDF_VALUE_STRING;
            value->val.string = strdup(_parse_cff_sid_to_string(0, string_index));
            charset_arr->add(value);
            for (int i = 1; i < count; i++)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                v->type = PDF_VALUE_STRING;
                const char* s = _parse_cff_sid_to_string(sid, string_index);
                v->val.string = strdup(s);
                charset_arr->add(v);
            }
        }
    }
    else if (format == 1)
    {
        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        if (isCIDFont)
        {
            value->type = PDF_VALUE_NUMBER;
            value->val.number = 0;
            charset_arr->add(value);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint8_t left = p[0]; p++;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    v->type = PDF_VALUE_NUMBER;
                    v->val.number = sid;
                    sid++;
                    charset_arr->add(v);
                    gid++;
                }
            }
        }
        else
        {
            value->type = PDF_VALUE_STRING;
            value->val.string = strdup(_parse_cff_sid_to_string(0, string_index));
            charset_arr->add(value);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint8_t left = p[0] << 8 | p[1]; p += 2;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    v->type = PDF_VALUE_STRING;
                    const char* s = _parse_cff_sid_to_string(sid, string_index);
                    v->val.string = strdup(s);
                    sid++;
                    charset_arr->add(v);
                    gid++;
                }
            }
        }
    }
    else if (format == 2)
    {
        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        if (isCIDFont)
        {
            value->type = PDF_VALUE_NUMBER;
            value->val.number = 0;
            charset_arr->add(value);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint16_t left = p[0]; p++;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    v->type = PDF_VALUE_NUMBER;
                    v->val.number = sid;
                    sid++;
                    charset_arr->add(v);
                    gid++;
                }
            }
        }
        else
        {
            value->type = PDF_VALUE_STRING;
            value->val.string = strdup(_parse_cff_sid_to_string(0, string_index));
            charset_arr->add(value);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint16_t left = p[0] << 8 | p[1]; p += 2;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    v->type = PDF_VALUE_STRING;
                    const char* s = _parse_cff_sid_to_string(sid, string_index);
                    v->val.string = strdup(s);
                    sid++;
                    charset_arr->add(v);
                    gid++;
                }
            }
        }
    }
    return charset_arr;
}

void pdf_cff_parse(pdf_font_descriptor_t* font_descriptor)
{
    if (font_descriptor == NULL || font_descriptor->fontfile == NULL) return;
    unsigned char* p = font_descriptor->fontfile + 4;
    unsigned char* end = font_descriptor->fontfile + font_descriptor->fontfile_len;
    
    pdf_array* name_index = _parse_cff_index(&p);
    delete name_index;

    pdf_array* top_dict_index = _parse_cff_index(&p);
    pdf_array* string_index = _parse_cff_index(&p);
    pdf_array* global_suber_index = _parse_cff_index(&p);
    pdf_dict* top_dict = new pdf_dict();
    _parse_cff_dict_data(font_descriptor->fontfile, (unsigned char*)top_dict_index->get(0)->val.string, 
        top_dict_index->get(0)->value_len, string_index, top_dict);
        
    int offCharStrings = top_dict->get_number("CharStrings");
    p = font_descriptor->fontfile + offCharStrings;
    pdf_array* charstrings_index = _parse_cff_index(&p);   
    font_descriptor->charstrings = charstrings_index;

    int offFDArray = top_dict->get_number("FDArray");
    p = font_descriptor->fontfile + offFDArray;
    pdf_array* font_dict_index = _parse_cff_index(&p);
    for (size_t i = 0; i < font_dict_index->size(); i++)
    {
        pdf_dict* font_dict = new pdf_dict();
        _parse_cff_dict_data(font_descriptor->fontfile, (unsigned char*)font_dict_index->get(i)->val.string, 
            font_dict_index->get(i)->value_len, string_index, font_dict);
        
        font_dict_index->get(i)->type = PDF_VALUE_DICT;
        free(font_dict_index->get(i)->val.string);
        font_dict_index->get(i)->val.dict = font_dict;
    }
    
    int offFDSelect = top_dict->get_number("FDSelect");
    p = font_descriptor->fontfile + offFDSelect;
    pdf_array* font_select = new pdf_array();
    /*
        the first element specifies format
        if == 0 : 
            fd = font_dict_select_arr[gid + 1];
        if == 3 :
            for i in ranges where i > 0:
                if font_dict_select_arr[i] <= gid <= font_dict_select_arr[i + 1]:
                    fd = font_dict_select_arr[i + 2]
    */
    if (p[0] == 0)
    {
        p++;
        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value->type = PDF_VALUE_NUMBER;
        value->val.number = 0;
        font_select->add(value);
        for (size_t i = 1; i < charstrings_index->size() + 1; i++)
        {
            pdf_value_t* v = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            v->type = PDF_VALUE_NUMBER;
            v->val.number = p[0];
            font_select->add(v);
            p++;
        }
    }
    else if (p[0] == 3)
    {
        p++;
        uint16_t ranges = p[1] << 8 | p[1]; p += 2;
        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value->type = PDF_VALUE_NUMBER;
        value->val.number = 3;
        font_select->add(value);

        uint16_t first = p[0] << 8 | p[1];
        uint8_t fd = p[2];
        uint16_t sentinel = p[3] << 8 | p[4];

        pdf_value_t* value1 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value1->type = PDF_VALUE_NUMBER;
        value1->val.number = first;
        font_select->add(value1);

        pdf_value_t* value2 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value2->type = PDF_VALUE_NUMBER;
        value2->val.number = sentinel - 1;
        font_select->add(value2);
        pdf_value_t* value3 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value3->type = PDF_VALUE_NUMBER;
        value3->val.number = fd;
        font_select->add(value3);
        p += 5;
        for (int i = 4; i < ranges * 3 + 1; )
        {
            first = sentinel;
            fd = p[0];
            sentinel = p[1] << 8 | p[2];

            pdf_value_t* valuei = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            valuei->type = PDF_VALUE_NUMBER;
            valuei->val.number = first;
            font_select->add(valuei);

            pdf_value_t* valuei1 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            valuei1->type = PDF_VALUE_NUMBER;
            valuei1->val.number = sentinel - 1;
            font_select->add(valuei1);
            
            pdf_value_t* valuei2 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            valuei2->type = PDF_VALUE_NUMBER;
            valuei2->val.number = fd;
            font_select->add(valuei2);

            p += 3;
            i += 3;
        }
    }
    font_descriptor->font_dict_arr = font_dict_index;
    font_descriptor->font_dict_select_arr = font_select;

    int type = top_dict->get_number("CharstringType");
    uint16_t global_bias;
    if (type == -1) type = 2;

    if (type == 1) global_bias = 0;
    else if (global_suber_index->size() < 1240) global_bias = 107;
    else if (global_suber_index->size() < 33900) global_bias = 1131;
    else global_bias = 32768;

    font_descriptor->global_subr = global_suber_index;
    font_descriptor->global_subr_bias = global_bias;

    pdf_array* fontmatrix = top_dict->get_array("FontMatrix");
    if (fontmatrix == NULL)
    {
        fontmatrix = new pdf_array();
        pdf_value_t* value0 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value0->type = PDF_VALUE_NUMBER;
        value0->val.number = 0.001;
        fontmatrix->add(value0);

        pdf_value_t* value1 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value1->type = PDF_VALUE_NUMBER;
        value1->val.number = 0;
        fontmatrix->add(value1);

        pdf_value_t* value2 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value2->type = PDF_VALUE_NUMBER;
        value2->val.number = 0;
        fontmatrix->add(value2);

        pdf_value_t* value3 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value3->type = PDF_VALUE_NUMBER;
        value3->val.number = 0.001;
        fontmatrix->add(value3);
        
        pdf_value_t* value4 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value4->type = PDF_VALUE_NUMBER;
        value4->val.number = 0;
        fontmatrix->add(value4);
        
        pdf_value_t* value5 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
        value5->type = PDF_VALUE_NUMBER;
        value5->val.number = 0.001;
        fontmatrix->add(value5);
    }
    font_descriptor->font_matrix = fontmatrix;
    // bool isCIDFont = pdf_dict_get_array(top_dict, "ROS") != NULL;
    // int offEncoding = pdf_dict_get_number(top_dict, "Encoding");
    
    // int offCharset = pdf_dict_get_number(top_dict, "charset");
    // pdf_array* charset_arr = NULL;
    // if (offCharset != -1)
    // {
    //     p = font->font_data + offCharset;
    //     charset_arr = _parse_cff_charset(p, charstrings_index->num_elements, string_index, isCIDFont);
    // }
    // delete charset_arr;
    delete top_dict;
    
    // delete charstrings_index;
    delete top_dict_index; 
    delete string_index;
    //delete global_suber_index;
}