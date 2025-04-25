#include "pdf.h"
#include "pdf-private.h"
#include "pdf-cff.h"

pdf_array_t* _parse_cff_index(unsigned char** p)
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
    pdf_array_t* arr = pdf_array_init();
    arr->num_elements = count;
    arr->values = (pdf_array_element_value_t**)malloc(arr->num_elements * sizeof(pdf_array_element_value_t*));
    for (int i = 1; i < count + 1; i++)
    {
        uint32_t len = offsets[i] - offsets[i - 1];
        arr->values[i - 1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        arr->values[i - 1]->type = STRING;
        arr->values[i - 1]->value_len = len;
        arr->values[i - 1]->val.string = (unsigned char*)malloc(len + 1);
        memcpy(arr->values[i - 1]->val.string, (*p), len);
        arr->values[i - 1]->val.string[len] = '\0';
        (*p) += len;
    }
    
    free(offsets);
    return arr;
}

char* _parse_cff_sid_to_string(uint16_t sid, pdf_array_t* string_index)
{
    if (sid >= 0 && sid <= CFF_NUM_STANDARD_STRINGS)
    {
        return CFF_STANDARD_STRINGS[sid];
    }
    else
    {
        return string_index->values[sid - CFF_NUM_STANDARD_STRINGS - 1]->val.string;
    }
}
void _parse_cff_try_skip_value(unsigned char** pp, pdf_deque_t* deque)
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
        pdf_deque_push(deque, &v, sizeof(double)); 
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
        pdf_deque_push(deque, &v, sizeof(double)); 
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
        v = strtod(buf, NULL);
        pdf_deque_push(deque, &v, sizeof(double)); 
    }
    else if (b0 >= 32 && b0 <= 246)
    {
        p++;
        v = b0 - 139;
        pdf_deque_push(deque, &v, sizeof(double)); 
    }
    else if (b0 >= 247 && b0 <= 250)
    {
        p++;
        uint8_t b1 = p[0];
        p++;
        v = (b0 - 247) * 256 + b1 + 108;
        pdf_deque_push(deque, &v, sizeof(double)); 
    }
    else if (b0 >= 251 && b0 <= 254)
    {
        p++;
        uint8_t b1 = p[0];
        p++;
        v = -(b0 - 251) * 256 - b1 - 108;
        pdf_deque_push(deque, &v, sizeof(double)); 
    }
    *pp = p;
}

void _parse_cff_dict_data(unsigned char* font_data, 
    unsigned char* p, int len, 
    pdf_array_t* string_index,
    pdf_dict_t* ret_dict)
{
    uint8_t data[16] = {0};
    pdf_node_t node;
    node.data = data;
    pdf_deque_t* deque = pdf_deque_init();
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
                char* names[] = {"version", "Notice", "FullName", "FamilyName", "Weight"};
                pdf_deque_pop_end(deque, &node);
                uint32_t sid = *((double*)data);
                pdf_dict_add(ret_dict, names[operator1], STRING, _parse_cff_sid_to_string(sid, string_index)); 
                p++;
                break;
            }
            case 5: // FontBBox
            {
                pdf_array_t* a = pdf_array_init();
                a->num_elements = 4;
                a->values = (pdf_array_element_value_t**)malloc(a->num_elements * sizeof(pdf_array_element_value_t*));
                for (int i = 0; i < 4; i++)
                {
                    pdf_deque_pop_end(deque, &node);
                    pdf_array_element_value_t* arr_v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    arr_v->type = NUMBER;
                    arr_v->val.number = *((double*)data);
                    a->values[i] = arr_v;
                }
                pdf_dict_add(ret_dict, "FontBBox", ARRAY, a);
                p++;
                break;
            }
            case 6://BlueValues
            case 7://OtherBlues
            case 8://FamilyBlues
            case 9://FamilyOtherBlues
            {
                pdf_deque_empty(deque);
                p++;
                break;
            }
            case 10://StdHW
            case 11://StdVW
            {
                char* names[] = {"StdHW", "StdVW"};
                pdf_deque_pop_end(deque, &node);
                pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                value->type = NUMBER;
                value->val.number = *((double*)data);
                pdf_dict_add_value(ret_dict, names[operator1 - 10], value);
                p++;
                break;
            }
            case 14: // XUID
            {
                pdf_deque_empty(deque);
                p++;
                break;
            }
            case 13: // UniqueID
            case 15: // charset
            case 16: // Encoding
            case 17: // CharStrings
            {
                char* names[] = {"UniqueID", "XUID", "charset", "Encoding", "CharStrings"};
                pdf_deque_pop_end(deque, &node);
                pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                value->type = NUMBER;
                value->val.number = *((double*)data);
                pdf_dict_add_value(ret_dict, names[operator1 - 13], value);
                p++;
                break;
            }
            case 18: // Private
            {
                
                pdf_deque_pop_end(deque, &node);
                uint32_t size = *((double*)data);
                pdf_deque_pop_end(deque, &node);
                uint32_t offset = *((double*)data);

                _parse_cff_dict_data(font_data, font_data + offset, size, string_index, ret_dict);
                p++;
                break;
            }
            case 19:// Subrs(local)
            case 20:// defaultWidthX
            case 21:// nominalWidthX
            {
                char* names[] = {"Subrs", "defaultWidthX", "nominalWidthX"};
                pdf_deque_pop_end(deque, &node);
                pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                value->type = NUMBER;
                value->val.number = *((double*)data);
                pdf_dict_add_value(ret_dict, names[operator1 - 19], value);
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
                    pdf_deque_empty(deque);
                    break;
                }
                case 1: // isFixedPitch
                {
                    p += 2;
                    pdf_deque_empty(deque);
                    break;
                }
                case 7: // FontMatrix
                {
                    pdf_array_t* fontmatrix = pdf_array_init();
                    fontmatrix->num_elements = 6;
                    fontmatrix->values = (pdf_array_element_value_t**)malloc(sizeof(pdf_array_element_value_t*) * fontmatrix->num_elements);

                    for (int i = 0; i < fontmatrix->num_elements; i++)
                    {
                        pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                        value->type = NUMBER;
                        pdf_deque_pop_end(deque, &node);
                        value->val.number = *((double*)data);
                        fontmatrix->values[i] = value;
                    }
                    pdf_dict_add_array(ret_dict, "FontMatrix", fontmatrix);
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
                    char* names[] = {"ItalicAngle", "UnderlinePosition", "UnderlineThickness", "PaintType",
                    "CharstringType", "FontMatrix", "StrokeWidth"};

                    pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    value->type = NUMBER;
                    pdf_deque_pop_end(deque, &node);
                    value->val.number = *((double*)data);
                    pdf_dict_add_value(ret_dict, names[operator2 - 2], value);
                    p += 2;
                    break;
                }
                case 9://BlueScale
                case 10://BlueShift
                case 11://BlueFuzz
                {
                    pdf_deque_empty(deque);
                    p += 2;
                    break;
                }
                case 12://StemSnapH
                case 13://StemSnapV
                {
                    pdf_deque_empty(deque);
                    p += 2;
                    break;
                }
                case 14: // ForceBold
                {
                    pdf_deque_empty(deque);
                    p += 2;
                    break;
                }
                case 17:// LangiageGroup
                case 18:// ExpansionFactor
                case 19://initialRandomSeed
                case 20: // SyntheticBase
                {
                    pdf_deque_empty(deque);
                    p += 2;
                    break;
                }
                case 21: // PostScript
                case 22: // BaseFontName
                case 23: // BaseFontBlend
                {
                    p += 2;
                    pdf_deque_empty(deque);
                    break;
                }
                case 30: // ROS
                {
                    
                    pdf_deque_pop_end(deque, &node);
                    uint16_t sid1 = *((double*)data);
                    pdf_deque_pop_end(deque, &node);
                    uint16_t sid2 = *((double*)data);
                    pdf_deque_pop_end(deque, &node);
                    double number = *((double*)data);

                    pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    value->type = ARRAY;
                    value->val.array = pdf_array_init();
                    value->val.array->num_elements = 3;
                    value->val.array->values = (pdf_array_element_value_t**)malloc(sizeof(pdf_array_element_value_t**) * 3);

                    value->val.array->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    value->val.array->values[0]->type = STRING;
                    char* s1 = _parse_cff_sid_to_string(sid1, string_index);
                    value->val.array->values[0]->val.string = strdup(s1);
                    
                    value->val.array->values[1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    value->val.array->values[1]->type = STRING;
                    char* s2 = _parse_cff_sid_to_string(sid2, string_index);
                    value->val.array->values[1]->val.string = strdup(s2);

                    value->val.array->values[2] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    value->val.array->values[2]->type = NUMBER;
                    value->val.array->values[2]->val.number = number;
                    pdf_dict_add_value(ret_dict, "ROS", value);

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
                    char* names[] = {"CIDFontVersion", "CIDFontRevision", "CIDFontType",
                    "CIDCount", "UIDBase", "FDArray", "FDSelect"};
                    
                    pdf_value_t* value = (pdf_value_t*)malloc(sizeof(pdf_value_t));
                    value->type = NUMBER;
                    pdf_deque_pop_end(deque, &node);
                    value->val.number = *((double*)data);
                    pdf_dict_add_value(ret_dict, names[operator2 - 31], value);
                    p += 2;

                    break;
                }
                case 38: // FontName
                {
                    p += 2;
                    pdf_deque_empty(deque);
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
    pdf_deque_free(deque);
    return ret_dict;
}
pdf_array_t* _parse_cff_charset(unsigned char* p, int count, pdf_array_t* string_index, bool isCIDFont)
{
    uint8_t format = p[0];p++;
    pdf_array_t* charset_arr = NULL;
    charset_arr = pdf_array_init();
    charset_arr->num_elements = count;
    charset_arr->values = (pdf_array_element_value_t**)malloc(sizeof(pdf_array_element_value_t*) * count);
    if (format == 0)
    {
        charset_arr->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        if (isCIDFont)
        {
            charset_arr->values[0]->type = NUMBER;
            charset_arr->values[0]->val.number = 0;
            for (int i = 1; i < count; i++)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                v->type = NUMBER;
                v->val.number = sid;
                charset_arr->values[i] = v;
            }
        }
        else
        {
            charset_arr->values[0]->type = STRING;
            charset_arr->values[0]->val.string = _parse_cff_sid_to_string(0, string_index);
            for (int i = 1; i < count; i++)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                v->type = STRING;
                char* s = _parse_cff_sid_to_string(sid, string_index);
                v->val.string = strdup(s);
                charset_arr->values[i] = v;
            }
        }
    }
    else if (format == 1)
    {
        charset_arr->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        if (isCIDFont)
        {
            charset_arr->values[0]->type = NUMBER;
            charset_arr->values[0]->val.number = 0;
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint8_t left = p[0]; p++;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    v->type = NUMBER;
                    v->val.number = sid;
                    sid++;
                    charset_arr->values[gid] = v;
                    gid++;
                }
            }
        }
        else
        {
            charset_arr->values[0]->type = STRING;
            charset_arr->values[0]->val.string = _parse_cff_sid_to_string(0, string_index);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint8_t left = p[0] << 8 | p[1]; p += 2;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    v->type = STRING;
                    char* s = _parse_cff_sid_to_string(sid, string_index);
                    v->val.string = strdup(s);
                    sid++;
                    charset_arr->values[gid] = v;
                    gid++;
                }
            }
        }
    }
    else if (format == 2)
    {
        charset_arr->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        if (isCIDFont)
        {
            charset_arr->values[0]->type = NUMBER;
            charset_arr->values[0]->val.number = 0;
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint16_t left = p[0]; p++;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    v->type = NUMBER;
                    v->val.number = sid;
                    sid++;
                    charset_arr->values[gid] = v;
                    gid++;
                }
            }
        }
        else
        {
            charset_arr->values[0]->type = STRING;
            charset_arr->values[0]->val.string = _parse_cff_sid_to_string(0, string_index);
            int gid = 1;
            while (gid < count)
            {
                uint16_t sid = p[0] << 8 | p[1]; p += 2;
                uint16_t left = p[0] << 8 | p[1]; p += 2;
                for (int i = 0; i < left + 1; i++)
                {
                    pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
                    v->type = STRING;
                    char* s = _parse_cff_sid_to_string(sid, string_index);
                    v->val.string = strdup(s);
                    sid++;
                    charset_arr->values[gid] = v;
                    gid++;
                }
            }
        }
    }
    return charset_arr;
}

void pdf_cff_parse(pdf_font_t* font)
{
    if (font == NULL || font->font_data == NULL) return;
    unsigned char* p = font->font_data + 4;
    unsigned char* end = font->font_data + font->font_data_length;
    
    pdf_array_t* name_index = _parse_cff_index(&p);
    pdf_array_free(name_index);

    pdf_array_t* top_dict_index = _parse_cff_index(&p);
    pdf_array_t* string_index = _parse_cff_index(&p);
    pdf_array_t* global_suber_index = _parse_cff_index(&p);
    pdf_dict_t* top_dict = pdf_dict_init();
    _parse_cff_dict_data(font->font_data, top_dict_index->values[0]->val.string, 
        top_dict_index->values[0]->value_len, string_index, top_dict);

    int offCharStrings = pdf_dict_get_number(top_dict, "CharStrings");
    p = font->font_data + offCharStrings;
    pdf_array_t* charstrings_index = _parse_cff_index(&p);   
    font->charstrings = charstrings_index;

    int offFDArray = pdf_dict_get_number(top_dict, "FDArray");
    p = font->font_data + offFDArray;
    pdf_array_t* font_dict_index = _parse_cff_index(&p);
    for (int i = 0; i < font_dict_index->num_elements; i++)
    {
        pdf_dict_t* font_dict = pdf_dict_init();
        _parse_cff_dict_data(font->font_data, font_dict_index->values[i]->val.string, 
            font_dict_index->values[i]->value_len, string_index, font_dict);
        
        font_dict_index->values[i]->type = DICT;
        free(font_dict_index->values[i]->val.string);
        font_dict_index->values[i]->val.dict = font_dict;
    }
    
    int offFDSelect = pdf_dict_get_number(top_dict, "FDSelect");
    p = font->font_data + offFDSelect;
    pdf_array_t* font_select = pdf_array_init();
    if (p[0] == 0)
    {
        p++;
        font_select->num_elements = charstrings_index->num_elements + 1;
        font_select->values = (pdf_array_element_value_t**)malloc(font_select->num_elements * sizeof(pdf_array_element_value_t*));
        font_select->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        font_select->values[0]->type = NUMBER;
        font_select->values[0]->val.number = 0;
        for (int i = 1; i < font_select->num_elements; i++)
        {
            font_select->values[i] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            font_select->values[i]->type = NUMBER;
            font_select->values[i]->val.number = p[0];
            p++;
        }
    }
    else if (p[0] == 3)
    {
        p++;
        uint16_t ranges = p[1] << 8 | p[1]; p += 2;
        font_select->num_elements = ranges * 3 + 1;
        font_select->values = (pdf_array_element_value_t**)malloc(font_select->num_elements * sizeof(pdf_array_element_value_t*));
        font_select->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        font_select->values[0]->type = NUMBER;
        font_select->values[0]->val.number = 3;

        uint16_t first = p[0] << 8 | p[1];
        uint8_t fd = p[2];
        uint16_t sentinel = p[3] << 8 | p[4];

        font_select->values[1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        font_select->values[1]->type = NUMBER;
        font_select->values[1]->val.number = first;
        font_select->values[2] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        font_select->values[2]->type = NUMBER;
        font_select->values[2]->val.number = sentinel - 1;
        font_select->values[3] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        font_select->values[3]->type = NUMBER;
        font_select->values[3]->val.number = fd;
        p += 5;
        for (int i = 4; i < font_select->num_elements; )
        {
            first = sentinel;
            fd = p[0];
            sentinel = p[1] << 8 | p[2];

            font_select->values[i] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            font_select->values[i]->type = NUMBER;
            font_select->values[i]->val.number = first;
            font_select->values[i + 1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            font_select->values[i + 1]->type = NUMBER;
            font_select->values[i + 1]->val.number = sentinel - 1;
            font_select->values[i + 2] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            font_select->values[i + 2]->type = NUMBER;
            font_select->values[i + 2]->val.number = fd;
            p += 3;
            i += 3;
        }
    }
    font->font_dict_arr = font_dict_index;
    font->font_dict_select_arr = font_select;

    int type = pdf_dict_get_number(top_dict, "CharstringType");
    uint16_t global_bias;
    if (type == -1) type = 2;

    if (type == 1) global_bias = 0;
    else if (global_suber_index->num_elements < 1240) global_bias = 107;
    else if (global_suber_index->num_elements < 33900) global_bias = 1131;
    else global_bias = 32768;

    font->global_subr = global_suber_index;
    font->global_subr_bias = global_bias;

    pdf_array_t* fontmatrix = pdf_dict_get_array(top_dict, "FontMatrix");
    if (fontmatrix == NULL)
    {
        fontmatrix = pdf_array_init();
        fontmatrix->num_elements = 6;
        fontmatrix->values = (pdf_array_element_value_t**)malloc(sizeof(pdf_array_element_value_t*) * fontmatrix->num_elements);
        fontmatrix->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[0]->type = NUMBER;
        fontmatrix->values[0]->val.number = 0.001;
        fontmatrix->values[1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[1]->type = NUMBER;
        fontmatrix->values[1]->val.number = 0;
        fontmatrix->values[2] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[2]->type = NUMBER;
        fontmatrix->values[2]->val.number = 0;
        fontmatrix->values[3] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[3]->type = NUMBER;
        fontmatrix->values[3]->val.number = 0.001;
        fontmatrix->values[4] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[4]->type = NUMBER;
        fontmatrix->values[4]->val.number = 0;
        fontmatrix->values[5] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
        fontmatrix->values[5]->type = NUMBER;
        fontmatrix->values[5]->val.number = 0.001;
    }
    font->font_matrix = fontmatrix;
    // bool isCIDFont = pdf_dict_get_array(top_dict, "ROS") != NULL;
    // int offEncoding = pdf_dict_get_number(top_dict, "Encoding");
    
    // int offCharset = pdf_dict_get_number(top_dict, "charset");
    // pdf_array_t* charset_arr = NULL;
    // if (offCharset != -1)
    // {
    //     p = font->font_data + offCharset;
    //     charset_arr = _parse_cff_charset(p, charstrings_index->num_elements, string_index, isCIDFont);
    // }
    // pdf_array_free(charset_arr);
    pdf_dict_free(top_dict);
    
    // pdf_array_free(charstrings_index);
    pdf_array_free(top_dict_index); 
    pdf_array_free(string_index);
    //pdf_array_free(global_suber_index);
}