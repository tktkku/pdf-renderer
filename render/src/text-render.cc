#include "pdf-private.h"
#include "pdf-render.h"
#include "pdf-render-private.h"
#include <stdint.h>
#include <wchar.h>
#include <iostream>
#include <assert.h>
#include "pdf-encoding.h"
#include "pdf-glyph-list.h"

uint32_t _convert_unicode_from_latin_encoding(uint16_t code, pdf_latin_encoding_type_t encoding)
{
    // ponytail: pre-built 256-entry direct lookup tables, built once per encoding type
    static uint32_t latin_table_std[256] = {};
    static uint32_t latin_table_mac[256] = {};
    static uint32_t latin_table_win[256] = {};
    static uint32_t latin_table_pdf[256] = {};
    static bool latin_tables_built = false;
    if (!latin_tables_built)
    {
        latin_tables_built = true;
        struct { const pdf_latin_encoding_map_t* map; int len; uint32_t* table; } configs[] = {
            {macRomanEncoding, 207, latin_table_std},
            {macRomanEncoding, 207, latin_table_mac},
            {winAnsiEncoding,  217, latin_table_win},
            {pdfDocEncoding,   229, latin_table_pdf},
        };
        for (auto& cfg : configs)
        {
            for (int i = 0; i < cfg.len; i++)
            {
                const char* name = cfg.map[i].name;
                int left = 0, right = ARRAY_COUNT(glyphlist) - 1;
                while (left <= right)
                {
                    int mid = left + (right - left) / 2;
                    int cmp = strcmp(glyphlist[mid].name, name);
                    if (cmp == 0) { cfg.table[cfg.map[i].code] = glyphlist[mid].unicode; break; }
                    else if (cmp < 0) left = mid + 1;
                    else right = mid - 1;
                }
            }
        }
    }
    uint32_t* table = NULL;
    switch (encoding)
    {
        case PDF_LATIN_ENCODING_STD: table = latin_table_std; break;
        case PDF_LATIN_ENCODING_MAC: table = latin_table_mac; break;
        case PDF_LATIN_ENCODING_WIN: table = latin_table_win; break;
        case PDF_LATIN_ENCODING_PDF: table = latin_table_pdf; break;
        default: return 0;
    }
    return table[code];
}
uint32_t _convert_code_from_cmap(pdf_cmap* cmap, uint32_t code)
{
    if (cmap != NULL)
    {
        // ponytail: binary search on sorted point map
        auto it = std::lower_bound(cmap->cid_map.begin(), cmap->cid_map.end(), code,
            [](const pdf_cmap_cid_map& m, uint32_t v) { return m.code < v; });
        if (it != cmap->cid_map.end() && it->code == code)
            return it->cid;

        // ponytail: binary search on sorted range map
        auto rit = std::lower_bound(cmap->cid_range_map.begin(), cmap->cid_range_map.end(), code,
            [](const pdf_cmap_char_range& r, uint32_t v) { return r.srcEnd < v; });
        if (rit != cmap->cid_range_map.end() && code >= rit->srcStart && code <= rit->srcEnd)
            return rit->dstStart + (code - rit->srcStart);
    }
    return code;
}
std::string codepoint_to_utf8(uint32_t cp)
{
    std::string result;
    if (cp <= 0x7F)
    {
        result += static_cast<char>(cp);
    }
    else if (cp <= 0x7FF)
    {
        result += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    else if (cp <= 0xFFFF)
    {
        if (cp >= 0xD800 && cp <= 0xDFFF) return "";
        result += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    else if (cp <= 0x10FFFF)
    {
        result += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
        result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (cp & 0x3F));
    }
    else
    {
        return "";
    }
    return result;
}
uint32_t _convert_unicode_from_cmap(pdf_cmap* cmap, uint32_t code)
{
    if (cmap != NULL)
    {
        // ponytail: binary search on sorted point map
        auto it = std::lower_bound(cmap->unicode_map.begin(), cmap->unicode_map.end(), code,
            [](const pdf_cmap_unicode_map& m, uint32_t v) { return m.code < v; });
        if (it != cmap->unicode_map.end() && it->code == code)
            return it->unicode;

        // ponytail: binary search on sorted range map
        auto rit = std::lower_bound(cmap->unicode_range_map.begin(), cmap->unicode_range_map.end(), code,
            [](const pdf_cmap_char_range& r, uint32_t v) { return r.srcEnd < v; });
        if (rit != cmap->unicode_range_map.end() && code >= rit->srcStart && code <= rit->srcEnd)
            return rit->dstStart + (code - rit->srcStart);
    }
    return code;
}

void _cff_do_render_char(pdf_cff_char_render_t* context, pdf_deque* deque)
{
    //pdf_array* charstrings_index = context->charstrings;
    //pdf_array* global_subr_index = context->global_subr;
    //uint16_t global_bias = context->global_bias;

    unsigned char* end = context->buf + context->len;

    // for (int i = 0; i < context->len; i++)
    // {
    //     printf("%d ", context->cur[i]);
    // }
    // printf("\n");

    //static double width = 0;
    while (context->cur < end)
    {
        double v = 0;
        uint8_t b0 = context->cur[0];
        if (b0 == 28)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            uint8_t b2 = context->cur[1];
            context->cur += 2;
            v = b1 << 8 | b2; 
            deque->push_front(&v, sizeof(double));
        }
        else if (b0 >= 32 && b0 <= 246)
        { 
            context->cur++;
            v = b0 - 139;
            deque->push_front(&v, sizeof(double));
        }
        else if (b0 >= 247 && b0 <= 250)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = (b0 - 247) * 256 + b1 + 108;
            deque->push_front(&v, sizeof(double));
        }
        else if (b0 >= 251 && b0 <= 254)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = -(b0 - 251) * 256 - b1 - 108;
            deque->push_front(&v, sizeof(double));
        }
        else if (b0 == 255)
        {
            context->cur++;
            int32_t raw_value = (context->cur[0] << 24) | (context->cur[1] << 16) | (context->cur[2] << 8) | context->cur[3];
            int32_t signed_value = (int32_t)raw_value;
            v = (float)signed_value / 65536.0;
            context->cur += 4;
            deque->push_front(&v, sizeof(double));
        } 
        else if (b0 == 11) break;
        else if (b0 == 14)
        {
            if (deque->size() > 0 && !context->havewidth)
            {
                auto data = deque->pop_back();
                context->width = *((double*)data->data());
                context->havewidth = true;
            }
            deque->clear();

            context->open = false;
            break;
        }

        uint8_t operator1 = context->cur[0];
        switch (operator1)
        {
            case 1://hstem
            case 3://vstem
            case 4://vmoveto
            case 5://rlineto
            case 6://hlineto
            case 7://vlineto
            case 8://rrcurveto
            case 10://callsubr
            case 18://hstemhm
            case 19://hintmask
            case 20://cntrmask
            case 21://rmoveto
            case 22://hmoveto
            case 23://vstemhm
            case 24://rcurveline
            case 25://rlinecurve
            case 26://vvcurveto
            case 27://hhcurveto
            case 29://callgsubr
            case 30://vhcurveto
            case 31://hvcurveto
            {
                CFF_HANDLERS1[operator1](context, deque);
                context->cur++;
                if (!context->fisr_stack_clear)
                {
                    if (operator1 != 10 && operator1 != 29)
                        deque->clear();
                }
                else
                {
                    if (operator1 == 1//hstem
                    || operator1 == 3//vstem
                    || operator1 == 4//vmoveto
                    || operator1 == 18//hstemhm
                    || operator1 == 19//hintmask
                    || operator1 == 20//cntrmask
                    || operator1 == 21//rmoveto
                    || operator1 == 22//hmoveto
                    || operator1 == 23//vstemhm
                    )
                    {
                        deque->clear();
                        context->fisr_stack_clear = false;
                        break;
                    }
                }
                break;
            }
            case 11://return
            {
                return;
            }
            case 14://endcar
            {
                if (deque->size() > 0 && !context->havewidth)
                {
                    auto data = deque->pop_back();
                    context->width = *((double*)data->data());
                    context->havewidth = true;
                }
                deque->clear();

                context->open = false;
                return;
            }
            case 12:
            {
                uint8_t operator2 = context->cur[1];
                switch (operator2)
                {
                    case 3: // and
                    case 4: // or
                    case 5:// not
                    case 9://abs
                    case 10: // add
                    case 11: // sub
                    case 12: // div
                    case 14: // neg
                    case 15://eq
                    case 18: // drop
                    case 20: // put
                    case 21: // get
                    case 22:// ifelse
                    case 23://random
                    case 24://mul
                    case 26: // sqrt
                    case 27: //dup
                    case 28: //exch
                    case 29: // index
                    case 30:// roll
                    case 34://hflex
                    case 35://flex
                    case 36: // hflex1
                    case 37://flex1
                    {
                        if (CFF_HANDLERS2[operator2])
                            CFF_HANDLERS2[operator2](context, deque);
                        context->cur += 2;
                        break;
                    }
                    default:
                        break;
                }
            }
            default:
                break;
        }
    }
}

typedef struct unicode_text
{
    uint32_t utf32;
    bool isGid;
} unicode_text_t;

float pdf_font_face_traverse_glyph_path(
    pdf_render* context, 
    float x, float y, 
    uint32_t codepoint, bool isGid)
{   
    pdf_font_face_t* face = context->state->textState.fontface;
    float size = context->state->textState.fontSize;
    float scale = stbtt_ScaleForMappingEmToPixels(&face->info, size);
    pdf_matrix_t matrix;
    pdf_matrix_init_translate(&matrix, x, y);
    pdf_matrix_scale(&matrix, scale, -scale);

    pdf_point_t points[3];
    pdf_point_t current_point = {0, 0};
    unsigned int msb = (codepoint >> 8) & 0xFF;
    if(face->glyphs[msb] == NULL) {
        face->glyphs[msb] = (glyph_t**)calloc(GLYPH_CACHE_SIZE, sizeof(glyph_t*));
    }

    unsigned int lsb = codepoint & 0xFF;
    if(face->glyphs[msb][lsb] == NULL) {
        glyph_t* glyph = new glyph_t{};
        if (isGid) 
            glyph->index = codepoint;
        else 
            glyph->index = stbtt_FindGlyphIndex(&face->info, codepoint);
        glyph->nvertices = stbtt_GetGlyphShape(&face->info, glyph->index, &glyph->vertices);
        stbtt_GetGlyphHMetrics(&face->info, glyph->index, &glyph->advance_width, &glyph->left_side_bearing);
        if(!stbtt_GetGlyphBox(&face->info, glyph->index, &glyph->x1, &glyph->y1, &glyph->x2, &glyph->y2))
            glyph->x1 = glyph->y1 = glyph->x2 = glyph->y2 = 0;
        face->glyphs[msb][lsb] = glyph;
    }

    glyph_t* glyph = face->glyphs[msb][lsb];
    for(int i = 0; i < glyph->nvertices; i++) {
        switch(glyph->vertices[i].type) {
        case STBTT_vmove:
            points[0].x = glyph->vertices[i].x;
            points[0].y = glyph->vertices[i].y;
            current_point = points[0];
            pdf_matrix_map_points(&matrix, points, points, 1);
            PDF_RENDERER_CALL(context->renderer, move_to, points[0].x, points[0].y);
            break;
        case STBTT_vline:
            points[0].x = glyph->vertices[i].x;
            points[0].y = glyph->vertices[i].y;
            current_point = points[0];
            pdf_matrix_map_points(&matrix, points, points, 1);
            PDF_RENDERER_CALL(context->renderer, line_to, points[0].x, points[0].y);
            break;
        case STBTT_vcurve:
            points[0].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * glyph->vertices[i].x;
            points[1].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * glyph->vertices[i].y;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            pdf_matrix_map_points(&matrix, points, points, 3);
            PDF_RENDERER_CALL(context->renderer, cubic_to, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        case STBTT_vcubic:
            points[0].x = glyph->vertices[i].cx;
            points[0].y = glyph->vertices[i].cy;
            points[1].x = glyph->vertices[i].cx1;
            points[1].y = glyph->vertices[i].cy1;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            pdf_matrix_map_points(&matrix, points, points, 3);
            PDF_RENDERER_CALL(context->renderer, cubic_to, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
            break;
        default:
            assert(false);
        }
    }

    return glyph->advance_width * scale;
}

float pdf_add_text(pdf_render* context, const unicode_text_t* text, int length, float x, float y)
{
    float advance_width = 0.f;
    double charSpace = context->state->textState.characterSpacing;
    double wordSpace = context->state->textState.wordSpacing;
    for (int i = 0; i < length; i++)
    {
        uint32_t codepoint = text[i].utf32;
        // ponytail: apply word spacing for space character (U+0020)
        if (codepoint == 32)
            advance_width += (float)wordSpace;
        float glyph_adv = pdf_font_face_traverse_glyph_path(
            context,
            x + advance_width, y, 
            codepoint,
            text[i].isGid);
        advance_width += glyph_adv;
        // ponytail: apply character spacing between glyphs (except after last)
        if (i < length - 1)
            advance_width += (float)charSpace;
    }
    return advance_width;
}

void _do_text_render(pdf_render* context, char* buf, int len)
{
#define DEBUG_TEXT 0
    if (context == NULL || context->state->textState.font == NULL || buf == NULL)
    {
        return;
    }
    std::vector<uint8_t> bytes;
    if (buf[0] == '<')
    {
        for (int i = 1; i < len; i += 2)
        {
            bytes.push_back(_hex_str_to_8bit(buf + i, 2));
        }
    }
    else
    {
        bytes.insert(bytes.end(), (uint8_t*)buf + 1, (uint8_t*)buf + len);
    }
    
    int unicode_cnt = 0;
    unicode_text_t unicode[1024] = { 0 };
    unsigned char* pbuf = (unsigned char*)buf;
    pdf_font_t* font = context->state->textState.font;
    if (font == NULL)
        return;

    if (font->subtype == FONT_SUBTYPE_TYPE3)
    {
        pdf_font_type3_t* type3 = font->type3;
        if (type3 == NULL)
        {
            return;
        }
        pdf_cmap* to_unicode_map = type3->to_unicode_map;
        for (size_t i = 0; i < bytes.size(); i++)
        {
            uint8_t code = bytes[i];
            unicode[unicode_cnt].utf32 = code;
            unicode[unicode_cnt].isGid = false;
            unicode_cnt++;
        }
        if (to_unicode_map != NULL)
        {
            for (size_t i = 0; i < bytes.size(); i++)
            {
                uint32_t code = bytes[i];
                
                for (size_t j = 0; j < to_unicode_map->code_range_map.size(); j++)
                {
                    if (code >= to_unicode_map->code_range_map[j].srcStart && code <= to_unicode_map->code_range_map[j].srcEnd)
                    {
                        uint32_t tmp = _convert_unicode_from_cmap(to_unicode_map, code);
                        wchar_t wc = tmp;
                        #if DEBUG_TEXT
                        printf("code = %d unicdoe = %d (%lc)\n", code, tmp, wc);
                        #endif
                        break;
                    }
                }
            }
        }
    }
    else if (font->subtype == FONT_SUBTYPE_TRUETYPE || font->subtype == FONT_SUBTYPE_TYPE1)
    {
        pdf_font_type1_t* type1_truetype = font->type1_truetype;
        if (type1_truetype == NULL)
        {
            return;
        }
        char* encoding = type1_truetype->encoding;
        pdf_cmap* to_unicode_map = type1_truetype->to_unicode_map;

        if (encoding != NULL)
        {
            pdf_latin_encoding_type_t latin_encoding = PDF_LATIN_ENCODING_STD;
            if (!strcmp(encoding, "/WinAnsiEncoding"))
            {
                latin_encoding = PDF_LATIN_ENCODING_WIN;
            }
            else if (!strcmp(encoding, "/MacRomanEncoding"))
            {
                latin_encoding = PDF_LATIN_ENCODING_MAC;
            }
            else if (!strcmp(encoding, "/PDFDocEncoding"))
            {
                latin_encoding = PDF_LATIN_ENCODING_PDF;
            }
            
            for (size_t i = 0; i < bytes.size(); i++)
            {
                uint16_t code = bytes[i];
                uint32_t uni = _convert_unicode_from_latin_encoding(code, latin_encoding);
                unicode[unicode_cnt].utf32 = uni;
                unicode[unicode_cnt].isGid = false;
                unicode_cnt++;
                wchar_t wc = uni;
                #if DEBUG_TEXT
                printf("code = %d unicode = %d (%lc)\n", code, uni, wc);
                #endif
            }
        }

        if (to_unicode_map != NULL)
        {
            // for (int i = 0; i < bytes.size(); i++)
            // {
            //     uint32_t code = bytes[i];
                
            //     for (int i = 0; i < to_unicode_map->code_range_map.size(); i++)
            //     {
            //         if (code >= to_unicode_map->code_range_map[i].srcStart && code <= to_unicode_map->code_range_map[i].srcEnd)
            //         {
            //             uint32_t tmp = _convert_code_from_cmap(to_unicode_map, code);
            //             wchar_t wc = tmp;
            //             printf("code = %d unicdoe = %d (%lc)\n", code, tmp, wc);
            //             break;
            //         }
            //     }
            // }
        }
    }
    else if (font->subtype == FONT_SUBTYPE_TYPE0)
    {
        pdf_font_type0_t* type0 = font->type0;
        if (type0 == NULL)
            return;
        pdf_font_t* descendant = type0->descendant;
        if (descendant == NULL)
        {
            return;
        }
        pdf_font_cidfont_t* cidfont = descendant->cidfont;
        if (cidfont == NULL)
        {
            return;
        }
        pdf_cmap* encoding = type0->encoding;
        pdf_cmap* to_unicode_map = type0->to_unicode_map;
        pdf_cmap* cid_to_gid_map = cidfont->cid_to_gid_map;
        // ponytail: helper to decode multi-byte CMap codes, shared by both CIDFont types
        auto decode_code = [&](size_t i, uint32_t& code, uint32_t& cid) -> size_t {
            for (int j = 4; j >= 1; j /= 2)
            {
                for (size_t k = 0; k < encoding->code_range_map.size(); k++)
                {
                    if (encoding->code_range_map[k].byte_len != j) continue;
                    uint32_t c = 0;
                    for (int l = j - 1; l >= 0; l--)
                    {
                        size_t idx = i + (j - 1 - l);
                        if (idx >= bytes.size()) break;
                        c = c << 8 | bytes[idx];
                    }
                    if (c >= encoding->code_range_map[k].srcStart && c <= encoding->code_range_map[k].srcEnd)
                    {
                        code = c;
                        cid = _convert_code_from_cmap(encoding, c);
                        return j;
                    }
                }
            }
            code = 0; cid = 0; return 0;
        };
        if (descendant->subtype == FONT_SUBTYPE_CIDFONTTPYE0)
        {
            for (size_t i = 0; i < bytes.size(); )
            {
                uint32_t code = 0, cid = 0;
                size_t advance = decode_code(i, code, cid);
                if (advance == 0) break;
                i += advance;
                uint32_t uni = 0;
                if (to_unicode_map != NULL)
                {
                    for (size_t j = 0; j < to_unicode_map->code_range_map.size(); j++)
                    {
                        if (code >= to_unicode_map->code_range_map[j].srcStart && code <= to_unicode_map->code_range_map[j].srcEnd)
                        {
                            uni = _convert_unicode_from_cmap(to_unicode_map, code);
                            wchar_t wc = uni;
                            #if DEBUG_TEXT
                            printf("unicode = %5d (%lc)\n", uni, wc);
                            #endif
                            break;
                        }
                    }
                }
                else if (encoding && encoding->name && !strncmp(encoding->name, "Uni", 3))
                {
                    uni = code;
                    wchar_t wc = uni;
                    #if DEBUG_TEXT
                    printf("unicode = %5d(%lc)\n", uni, wc);
                    #endif
                }
                else
                {
                    #if DEBUG_TEXT
                    printf("\n");
                    #endif
                }

                unicode[unicode_cnt].utf32 = context->state->textState.font_face_loaded ? cid : uni;
                unicode[unicode_cnt].isGid = context->state->textState.font_face_loaded;
                unicode_cnt++;
            }
        }
        else // if (descendant->subtype == FONT_SUBTYPE_CIDFONTTPYE2)
        {
            for (size_t i = 0; i < bytes.size(); )
            {
                uint32_t code = 0, cid = 0, gid = 0, uni = 0;
                size_t advance = decode_code(i, code, cid);
                if (advance == 0) break;
                i += advance;
                if (cid_to_gid_map != NULL)
                {
                    for (size_t j = 0; j < cid_to_gid_map->code_range_map.size(); j++)
                    {
                        if (cid >= cid_to_gid_map->code_range_map[j].srcStart && cid <= cid_to_gid_map->code_range_map[j].srcEnd)
                        {
                            gid = _convert_code_from_cmap(cid_to_gid_map, cid);
                            #if DEBUG_TEXT
                            printf("gid = %5d ", gid);
                            #endif
                            break;
                        }
                    }
                }
                else
                {
                    char to_unicode_name[64] = {0};
                    sprintf(to_unicode_name, "Adobe-%s-UCS2", cidfont->cid_system_info.ordering);
                    pdf_cmap* c = pdf_file_get_cmap(context->pdf, to_unicode_name);
                    if (c != NULL)
                    {
                        for (size_t j = 0; j < c->code_range_map.size(); j++)
                        {
                            if (cid >= c->code_range_map[j].srcStart && cid <= c->code_range_map[j].srcEnd)
                            {
                                uni = _convert_unicode_from_cmap(c, cid);
                                wchar_t wc = uni;
                                #if DEBUG_TEXT
                                printf("unicode = %5d (%lc)", uni, wc);
                                #endif
                                
                                break;
                            }
                        }
                    }
                }
                if (to_unicode_map != NULL)
                {
                    for (size_t j = 0; j < to_unicode_map->code_range_map.size(); j++)
                    {
                        if (code >= to_unicode_map->code_range_map[j].srcStart && code <= to_unicode_map->code_range_map[j].srcEnd)
                        {
                            uni = _convert_unicode_from_cmap(to_unicode_map, code);
                            wchar_t wc = uni;
                            #if DEBUG_TEXT
                            printf("unicode = %5d (%lc)\n", uni, wc);
                            #endif
                            break;
                        }
                    }
                }
                else
                {
                    #if DEBUG_TEXT
                    printf("\n");
                    #endif
                }

                unicode[unicode_cnt].utf32 = context->state->textState.font_face_loaded ? gid : uni;
                unicode[unicode_cnt].isGid = context->state->textState.font_face_loaded;
                unicode_cnt++;
            }
        }
    }

    if (unicode_cnt == 0)
    {
        return;
    }
    PDF_RENDERER_CALL(context->renderer, save);
    if (context->state->textState.font->subtype != FONT_SUBTYPE_TYPE3)
    {
        if (context->state->textState.fontface != NULL)
        {
            float x, y;
            PDF_RENDERER_CALL(context->renderer, get_current_point, &x, &y);
            // cos(theta), sin(theta),  0
            // -sin(theta), cos(theta), 0
            // 0, 0, 1

            // 1,  0, 0
            // 0, -1, 0,
            // 0,  0, 1
            // rotate 180閹�?
            // or scale by 1
            PDF_RENDERER_CALL(context->renderer, transform, 
                context->state->textState.textMatrix.a, context->state->textState.textMatrix.b, 
                context->state->textState.textMatrix.c, context->state->textState.textMatrix.d, 
                context->state->textState.textMatrix.e, context->state->textState.textMatrix.f);
            PDF_RENDERER_CALL(context->renderer, scale, 1, -1);
            PDF_RENDERER_CALL(context->renderer, set_line_width, context->state->lineWidth);
            PDF_RENDERER_CALL(context->renderer, set_miter_limit, context->state->miterLimit);
            PDF_RENDERER_CALL(context->renderer, set_line_cap, context->state->lineCap);
            PDF_RENDERER_CALL(context->renderer, set_line_join, context->state->lineJoin);

            PDF_RENDERER_CALL(context->renderer, new_path);
            float advance_width = 0;
            advance_width = pdf_add_text(context, unicode, unicode_cnt, context->state->textState.textLineWidth, 0);
            context->state->textState.textLineWidth += advance_width;

            // Check ForceBold flag in FontDescriptor (bit 19, 0x40000)
            // When set, simulate bold by also stroking the glyph outlines
            bool forceBold = false;
            {
                pdf_font_t* f = context->state->textState.font;
                pdf_font_descriptor_t* fd = NULL;
                if (f->subtype == FONT_SUBTYPE_TYPE0 && f->type0->descendant && f->type0->descendant->cidfont)
                    fd = f->type0->descendant->cidfont->font_descriptor;
                else if ((f->subtype == FONT_SUBTYPE_TYPE1 || f->subtype == FONT_SUBTYPE_TRUETYPE) && f->type1_truetype)
                    fd = f->type1_truetype->font_descriptor;
                else if (f->subtype == FONT_SUBTYPE_TYPE3 && f->type3)
                    fd = f->type3->font_descriptor;
                if (fd && (fd->flags & 0x40000))
                    forceBold = true;
            }

            int textMode = context->state->textState.textMode;
            if (forceBold && (textMode == 0 || textMode == 4))
            {
                // Simulate bold: fill + stroke with a small line width
                double savedLineWidth = context->state->lineWidth;
                double boldLineWidth = context->state->textState.fontSize * 0.025;
                if (boldLineWidth < 0.5) boldLineWidth = 0.5;
                PDF_RENDERER_CALL(context->renderer, set_line_width, boldLineWidth);
                PDF_RENDERER_CALL(context->renderer, set_line_cap, 1);  // round cap
                PDF_RENDERER_CALL(context->renderer, set_line_join, 1); // round join
                do_path(context, PDF_OPERATION_PATH_FILL | PDF_OPERATION_PATH_STROKE);
                PDF_RENDERER_CALL(context->renderer, set_line_width, savedLineWidth);
                PDF_RENDERER_CALL(context->renderer, set_line_cap, context->state->lineCap);
                PDF_RENDERER_CALL(context->renderer, set_line_join, context->state->lineJoin);
            }
            else switch (textMode)
            {
                case 0:// fill
                case 4:
                    do_path(context, PDF_OPERATION_PATH_FILL);
                    break;
                case 1: // stroke
                case 5:
                    do_path(context, PDF_OPERATION_PATH_STROKE);
                    break;
                case 2: // fill and then stroke
                case 6:
                    do_path(context, PDF_OPERATION_PATH_FILL 
                        | PDF_OPERATION_PATH_STROKE);
                    break;
                default:
                    break;
            }            
        }
        else
        {
            pdf_font_descriptor_t* font_descriptor = NULL;
            if (font->subtype == FONT_SUBTYPE_TYPE0)
            {
                font_descriptor = font->type0->descendant->cidfont->font_descriptor;
            }
            else if (font->subtype == FONT_SUBTYPE_TYPE1 || font->subtype == FONT_SUBTYPE_TRUETYPE)
            {
                font_descriptor = font->type1_truetype->font_descriptor;
            }
            else if (font->subtype == FONT_SUBTYPE_TYPE3)
            {
                font_descriptor = font->type3->font_descriptor;
            }
            if (font_descriptor == NULL) 
                return;
            pdf_array* charstrings_index = font_descriptor->charstrings;
            pdf_array* font_dict_aar = font_descriptor->font_dict_arr;
            pdf_array* font_dict_select = font_descriptor->font_dict_select_arr;
            if (charstrings_index != NULL)
            {
                pdf_array* font_matrix = font_descriptor->font_matrix;
                pdf_matrix_t original_matrix = context->state->textState.textMatrix;
                for (int i = 0; i < unicode_cnt; i++)
                {
                    pdf_deque* deque = new pdf_deque();
                    PDF_RENDERER_CALL(context->renderer, save);
                    pdf_matrix_t font_matrix_tmp, rm;
                    pdf_matrix_init(&font_matrix_tmp, 
                        font_matrix->get(0)->val.number, font_matrix->get(1)->val.number,
                        font_matrix->get(2)->val.number, font_matrix->get(3)->val.number,
                        font_matrix->get(4)->val.number, font_matrix->get(5)->val.number);
                    pdf_matrix_multiply(&rm, &font_matrix_tmp, &context->state->textState.textMatrix);
                    pdf_matrix_t m = {
                        (float)(context->state->textState.fontSize * context->state->textState.horizontalScaling / 100.0f), 0,
                        0, (float)context->state->textState.fontSize,
                        0, (float)context->state->textState.textRise
                    };
                    pdf_matrix_multiply(&rm, &m, &rm);
                    PDF_RENDERER_CALL(context->renderer, transform, 
                        rm.a, rm.b, rm.c, rm.d, rm.e, rm.f);
                    PDF_RENDERER_CALL(context->renderer, set_color, 
                        context->state->fill.color[0],
                        context->state->fill.color[1],
                        context->state->fill.color[2]);
                    PDF_RENDERER_CALL(context->renderer, new_path);
                    pdf_cff_char_render_t ctx;
                    ctx.buf = (unsigned char*)charstrings_index->get(unicode[i].utf32)->val.string;
                    ctx.cur = ctx.buf;
                    ctx.len = charstrings_index->get(unicode[i].utf32)->value_len;
                    ctx.renderer = context->renderer;
                    ctx.fontSize = context->state->textState.fontSize;
                    ctx.charstrings = font_descriptor->charstrings;
                    ctx.global_subr = font_descriptor->global_subr;
                    ctx.global_bias = font_descriptor->global_subr_bias;
                    ctx.open = false;
                    ctx.havewidth = false;
                    ctx.width = 0;
                    ctx.curX = 0;
                    ctx.curY = 0;
                    ctx.stems = 0;
                    ctx.stemshm = 0;
                    ctx.fisr_stack_clear = true;
                    _cff_do_render_char(&ctx, deque);
                    pdf_dict* font_dict = NULL;
                    double defaultWidthX = 0;
                    double nominalWidthX = 0;
                    if ((int)(font_dict_select->get(0)->val.number) == 0)
                    {
                        int fd = font_dict_select->get(unicode[i].utf32 + 1)->val.number;
                        font_dict = font_dict_aar->get(fd)->val.dict;
                    }
                    else
                    {
                        for (size_t j = 1; j < font_dict_select->size(); j += 3)
                        {
                            if (unicode[i].utf32 >= (uint32_t)((int)(font_dict_select->get(j)->val.number))
                            && unicode[i].utf32 <= (uint32_t)((int)(font_dict_select->get(j + 1)->val.number)))
                            {
                                int fd = (int)(font_dict_select->get(j + 2)->val.number);
                                font_dict = font_dict_aar->get(fd)->val.dict;
                                break;
                            }
                        }
                    }
                    if (font_dict != NULL)
                    {
                        defaultWidthX = font_dict->get_number("defaultWidthX");
                        nominalWidthX = font_dict->get_number("nominalWidthX");
                    }
                    double advance = defaultWidthX;
                    if ((int)(ctx.width) != 0)
                    {
                        advance = (ctx.width + nominalWidthX) * context->state->textState.fontSize / 1000.0;
                    }

                    context->state->textState.textLineWidth += advance;
                    pdf_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                    // ForceBold simulation for CFF fonts
                    if (font_descriptor && (font_descriptor->flags & 0x40000) 
                        && (context->state->textState.textMode == 0 || context->state->textState.textMode == 4))
                    {
                        double savedLineWidth = context->state->lineWidth;
                        double boldLineWidth = context->state->textState.fontSize * 0.025;
                        if (boldLineWidth < 0.5) boldLineWidth = 0.5;
                        PDF_RENDERER_CALL(context->renderer, set_line_width, boldLineWidth);
                        PDF_RENDERER_CALL(context->renderer, set_line_cap, 1);
                        PDF_RENDERER_CALL(context->renderer, set_line_join, 1);
                        PDF_RENDERER_CALL(context->renderer, fill);
                        PDF_RENDERER_CALL(context->renderer, stroke);
                        PDF_RENDERER_CALL(context->renderer, set_line_width, savedLineWidth);
                        PDF_RENDERER_CALL(context->renderer, set_line_cap, context->state->lineCap);
                        PDF_RENDERER_CALL(context->renderer, set_line_join, context->state->lineJoin);
                    }
                    else
                    {
                        PDF_RENDERER_CALL(context->renderer, fill);
                    }
                    PDF_RENDERER_CALL(context->renderer, restore); 
                    delete deque;
                }
                context->state->textState.textMatrix = original_matrix;
            }
        }
    }
    else
    {
        pdf_font_type3_t* type3 = context->state->textState.font->type3;
        float x, y;
        PDF_RENDERER_CALL(context->renderer, get_current_point, &x, &y);
        // render Type3 font
        pdf_array* differences = type3->differences;
        if (differences != NULL)
        {
            // pdf_array* font_bbox = type3->font_bbox;
            pdf_array* font_matrix = type3->font_matrix;
            pdf_array* widths = type3->widths;
            pdf_matrix_t original_matrix = context->state->textState.textMatrix;
            for (int i = 0; i < unicode_cnt; i++) 
            {
                uint32_t c = unicode[i].utf32;
                pdf_obj_t* obj = NULL;
                // ponytail: O(1) glyph cache avoids re-searching differences + re-resolving obj
                auto* cache = type3->glyph_cache;
                if (cache)
                {
                    auto it = cache->find(c);
                    if (it != cache->end()) { obj = it->second; goto RENDER_TYPE3_GLYPH; }
                }
                for (size_t j = 0; j < differences->size(); j += 2) 
                {
                    if ((uint32_t)differences->get(j)->val.number == c) 
                    {
                        const char* name = differences->get(j + 1)->val.name;
                        pdf_indirect_t indirect = type3->charProcs->get_indirect(name);
                        if (indirect.obj_num != -1) 
                        {
                            obj = pdf_file_get_obj(context->pdf, indirect);
                            if (cache) (*cache)[c] = obj;
                        }
                        break;
                    }
                }
            RENDER_TYPE3_GLYPH:
                if (obj != NULL && obj->stream != NULL) 
                            {
                                pdf_obj_t* save_obj = context->current_obj;
                                context->current_obj = obj;

                                PDF_RENDERER_CALL(context->renderer, save);
                                pdf_matrix_t font_matrix_tmp, rm;
                                pdf_matrix_init(&font_matrix_tmp, 
                                    font_matrix->get(0)->val.number, font_matrix->get(1)->val.number,
                                    font_matrix->get(2)->val.number, font_matrix->get(3)->val.number,
                                    font_matrix->get(4)->val.number, font_matrix->get(5)->val.number);
                                pdf_matrix_multiply(&rm, &font_matrix_tmp, &context->state->textState.textMatrix);
                                pdf_matrix_t m = {
                                    (float)(context->state->textState.fontSize * context->state->textState.horizontalScaling / 100.0f), 0,
                                    0, (float)context->state->textState.fontSize,
                                    0, (float)context->state->textState.textRise
                                };
                                pdf_matrix_multiply(&rm, &m, &rm);
                                PDF_RENDERER_CALL(context->renderer, transform, 
                                    rm.a, rm.b, rm.c, rm.d, rm.e, rm.f);
                                if (widths != NULL)
                                {
                                    float width = 0;
                                    if (c >= (uint32_t)type3->first_char && c <= (uint32_t)type3->last_char)
                                    {
                                        width = widths->get(c - type3->first_char)->val.number;
                                    }
                                    double advance = width * context->state->textState.fontSize / 1000.0;
                                    context->state->textState.textLineWidth += advance;
                                    pdf_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                                }
                                PDF_RENDERER_CALL(context->renderer, set_color,
                                    context->state->fill.color[0],
                                    context->state->fill.color[1],
                                    context->state->fill.color[2]);

                                PDF_RENDERER_CALL(context->renderer, new_path);
                                pdf_stream_open(obj->stream);
                                pdf_token* tk = NULL;
                                while ((tk = pdf_parser_next_token(obj->stream->parser)) != NULL) 
                                {
                                    if (tk->type() == TOKEN_STREAM_END)
                                        break;
                                    _do_render_operation(obj->stream, context, tk);
                                    delete tk;
                                }
                                pdf_stream_close(obj->stream);
                                context->current_obj = save_obj;
                                
                                PDF_RENDERER_CALL(context->renderer, restore);
                            }
            }
            context->state->textState.textMatrix = original_matrix;
        }
    }
    PDF_RENDERER_CALL(context->renderer, restore);
}
