#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
#include "plutovg-private.h"
#include <stdint.h>
#include <wchar.h>
#include <iostream>
#include <assert.h>
#include "pdf-encoding.h"

uint32_t _convert_unicode_from_latin_encoding(uint16_t code, pdf_latin_encoding_type_t encoding)
{
    const pdf_latin_encoding_map_t* map = NULL;
    switch (encoding)
    {
        case PDF_LATIN_ENCODING_STD:
            map = macRomanEncoding;
            break;
        case PDF_LATIN_ENCODING_MAC:
            map = macRomanEncoding;
            break;
        case PDF_LATIN_ENCODING_WIN:
            map = winAnsiEncoding;
            break;
        case PDF_LATIN_ENCODING_PDF:
            map = pdfDocEncoding;
            break;
        default:
            return 0;
    }
    if (map == NULL) return 0;
    int len = ARRAY_COUNT(map);
    const char* name = NULL;
    for (int i = 0; i < len; i++)
    {
        if (map[i].code == code)
        {
            name = map[i].name;
            break;
        }
    }
    if (name == NULL) return 0;
    FILE * f = fopen("agl-aglfn/glyphlist.txt", "r");
    if (f == NULL) return 0;
    char line[256] = {0};
    uint32_t unicode = 0;
    while (fgets(line, sizeof(line), f))
    {   
        if (line[0] == '#' || line[0] == '\n') continue;
        char* token = strtok(line, ";");
        if (token != NULL && strcmp(token, name) == 0)
        {            
            token = strtok(NULL, ";");
            if (token != NULL)            {
                unicode = strtoul(token, NULL, 16);
                break;
            }
        }
    }
    fclose(f);
    return unicode;
}
uint32_t _convert_code_from_cmap(pdf_cmap_t* cmap, uint32_t code)
{
    if (cmap != NULL)
    {
        for (int k = 0; k < cmap->cid_map.size(); k++)
        {
            if (code == cmap->cid_map[k].code)
            {
                return cmap->cid_map[k].cid;
            }
        }
 
        for (int k = 0; k < cmap->cid_range_map.size(); k++)
        {
            if (code >= cmap->cid_range_map[k].srcStart &&
                code <= cmap->cid_range_map[k].srcEnd)
            {
                return cmap->cid_range_map[k].dstStart +
                    (code - cmap->cid_range_map[k].srcStart);
            }
        }
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
uint32_t _convert_unicode_from_cmap(pdf_cmap_t* cmap, uint32_t code)
{
    if (cmap != NULL)
    {
        for (int k = 0; k < cmap->unicode_map.size(); k++)
        {
            if (code == cmap->unicode_map[k].code)
            {
                return cmap->unicode_map[k].unicode;
            }
        }
 
        for (int k = 0; k < cmap->unicode_range_map.size(); k++)
        {
            if (code >= cmap->unicode_range_map[k].srcStart &&
                code <= cmap->unicode_range_map[k].srcEnd)
            {
                return cmap->unicode_range_map[k].dstStart +
                    (code - cmap->unicode_range_map[k].srcStart);
            }
        }
    }
    return code;
}

void _cff_do_render_char(pdf_cff_char_render_t* context, pdf_deque_t* deque)
{
    //plutovg_canvas_t* canvas = context->canvas;
    //pdf_array_t* charstrings_index = context->charstrings;
    //pdf_array_t* global_subr_index = context->global_subr;
    //uint16_t global_bias = context->global_bias;

    unsigned char* end = context->buf + context->len;

    // for (int i = 0; i < context->len; i++)
    // {
    //     printf("%d ", context->cur[i]);
    // }
    // printf("\n");

    pdf_node_t node;
    char data[16] = {0};
    node.data = data;
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
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 32 && b0 <= 246)
        { 
            context->cur++;
            v = b0 - 139;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 247 && b0 <= 250)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = (b0 - 247) * 256 + b1 + 108;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 >= 251 && b0 <= 254)
        {
            context->cur++;
            uint8_t b1 = context->cur[0];
            context->cur++;
            v = -(b0 - 251) * 256 - b1 - 108;
            pdf_deque_push(deque, &v, sizeof(double));
        }
        else if (b0 == 255)
        {
            context->cur++;
            int32_t raw_value = (context->cur[0] << 24) | (context->cur[1] << 16) | (context->cur[2] << 8) | context->cur[3];
            int32_t signed_value = (int32_t)raw_value;
            v = (float)signed_value / 65536.0;
            context->cur += 4;
            pdf_deque_push(deque, &v, sizeof(double));
        } 
        else if (b0 == 11) break;
        else if (b0 == 14)
        {
            if (deque->size > 0 && !context->havewidth)
            {
                pdf_deque_pop_end(deque, &node);
                context->width = *((double*)data);
                context->havewidth = true;
            }
            pdf_deque_empty(deque);
            // if (context->open)
            //     plutovg_canvas_close_path(context->canvas);
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
                        pdf_deque_empty(deque);
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
                        pdf_deque_empty(deque);
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
                if (deque->size > 0 && !context->havewidth)
                {
                    pdf_deque_pop_end(deque, &node);
                    context->width = *((double*)data);
                    context->havewidth = true;
                }
                pdf_deque_empty(deque);
                // if (context->open)
                //     plutovg_canvas_close_path(context->canvas);
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
    union {
        uint8_t utf8;
        uint16_t utf16;
        uint32_t utf32;
    };
    plutovg_text_encoding_t encoding;
    bool isGid;
} unicode_text_t;

float plutovg_font_face_traverse_glyph_path1(plutovg_font_face_t* face, 
    float size, 
    float x, float y, 
    plutovg_codepoint_t codepoint, bool isGid,
    plutovg_path_traverse_func_t traverse_func, void* closure)
{
    float scale = stbtt_ScaleForMappingEmToPixels(&face->info, size);
    plutovg_matrix_t matrix;
    plutovg_matrix_init_translate(&matrix, x, y);
    plutovg_matrix_scale(&matrix, scale, -scale);

    plutovg_point_t points[3];
    plutovg_point_t current_point = {0, 0};
    unsigned int msb = (codepoint >> 8) & 0xFF;
    if(face->glyphs[msb] == NULL) {
        face->glyphs[msb] = (glyph_t**)calloc(GLYPH_CACHE_SIZE, sizeof(glyph_t*));
    }

    unsigned int lsb = codepoint & 0xFF;
    if(face->glyphs[msb][lsb] == NULL) {
        glyph_t* glyph = (glyph_t*)malloc(sizeof(glyph_t));
        if (isGid) glyph->index = codepoint;
        else glyph->index = stbtt_FindGlyphIndex(&face->info, codepoint);
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
            plutovg_matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_MOVE_TO, points, 1);
            break;
        case STBTT_vline:
            points[0].x = glyph->vertices[i].x;
            points[0].y = glyph->vertices[i].y;
            current_point = points[0];
            plutovg_matrix_map_points(&matrix, points, points, 1);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_LINE_TO, points, 1);
            break;
        case STBTT_vcurve:
            points[0].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * current_point.x;
            points[0].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * current_point.y;
            points[1].x = 2.f / 3.f * glyph->vertices[i].cx + 1.f / 3.f * glyph->vertices[i].x;
            points[1].y = 2.f / 3.f * glyph->vertices[i].cy + 1.f / 3.f * glyph->vertices[i].y;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            plutovg_matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        case STBTT_vcubic:
            points[0].x = glyph->vertices[i].cx;
            points[0].y = glyph->vertices[i].cy;
            points[1].x = glyph->vertices[i].cx1;
            points[1].y = glyph->vertices[i].cy1;
            points[2].x = glyph->vertices[i].x;
            points[2].y = glyph->vertices[i].y;
            current_point = points[2];
            plutovg_matrix_map_points(&matrix, points, points, 3);
            traverse_func(closure, PLUTOVG_PATH_COMMAND_CUBIC_TO, points, 3);
            break;
        default:
            assert(false);
        }
    }

    return glyph->advance_width * scale;
}
static void glyph_traverse_func(void* closure, plutovg_path_command_t command, const plutovg_point_t* points, int npoints)
{
    plutovg_path_t* path = (plutovg_path_t*)(closure);
    switch(command) {
    case PLUTOVG_PATH_COMMAND_MOVE_TO:
        plutovg_path_move_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_LINE_TO:
        plutovg_path_line_to(path, points[0].x, points[0].y);
        break;
    case PLUTOVG_PATH_COMMAND_CUBIC_TO:
        plutovg_path_cubic_to(path, points[0].x, points[0].y, points[1].x, points[1].y, points[2].x, points[2].y);
        break;
    case PLUTOVG_PATH_COMMAND_CLOSE:
        assert(false);
    }
}

float plutovg_canvas_add_text1(plutovg_canvas_t* canvas, const void* text, int length, plutovg_text_encoding_t encoding, float x, float y, bool isGid)
{
    plutovg_state_t* state = canvas->state;
    if(state->font_face == NULL || state->font_size <= 0.f)
        return 0.f;
    plutovg_text_iterator_t it;
    plutovg_text_iterator_init(&it, text, length, encoding);
    float advance_width = 0.f;
    while(plutovg_text_iterator_has_next(&it)) {
        plutovg_codepoint_t codepoint = plutovg_text_iterator_next(&it);
        advance_width += plutovg_font_face_traverse_glyph_path1(
            state->font_face, 
            state->font_size, 
            x + advance_width, y, 
            codepoint,
            isGid, 
            glyph_traverse_func, 
            canvas->path);
    }

    return advance_width;
}

void _do_text_render(pdf_render_t* context, char* buf, int len)
{
    if (context == NULL || context->state->textState.font == NULL || buf == NULL)
    {
        return;
    }
    std::vector<uint8_t> bytes;
    if (buf[0] == '<')
    {
        for (int i = 1; i < len; i += 2)
        {
            bytes.push_back(_hex_str_to_8bit(buf + i));
        }
    }
    else
    {
        bytes.insert(bytes.end(), (uint8_t*)buf + 1, (uint8_t*)buf + len);
    }
    plutovg_canvas_save(context->canvas);
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
        pdf_cmap_t* to_unicode_map = type3->to_unicode_map;
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
    else if (font->subtype == FONT_SUBTYPE_TRUETYPE || font->subtype == FONT_SUBTYPE_TYPE1)
    {
        pdf_font_type1_t* type1_truetype = font->type1_truetype;
        if (type1_truetype == NULL)
        {
            return;
        }
        char* encoding = type1_truetype->encoding;
        pdf_cmap_t* to_unicode_map = type1_truetype->to_unicode_map;

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
            
            for (int i = 0; i < bytes.size(); i++)
            {
                uint16_t code = bytes[i];
                uint32_t uni = _convert_unicode_from_latin_encoding(code, latin_encoding);
                unicode[unicode_cnt].utf8 = uni;
                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF32;
                unicode[unicode_cnt].isGid = false;
                unicode_cnt++;
                wchar_t wc = uni;
                printf("code = %d unicode = %d (%lc)\n", code, uni, wc);
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
        pdf_cmap_t* encoding = type0->encoding;
        pdf_cmap_t* to_unicode_map = type0->to_unicode_map;
        pdf_cmap_t* cid_to_gid_map = cidfont->cid_to_gid_map;
        if (descendant->subtype == FONT_SUBTYPE_CIDFONTTPYE0)
        {
            for (int i = 0; i < bytes.size(); )
            {
                uint32_t code = 0;
                uint32_t cid = 0;
                for (int j = 4; j >= 1; j /= 2)
                {
                    bool found = false;
                    for (int k = 0; k < encoding->code_range_map.size(); k++)
                    {
                        if (encoding->code_range_map[k].byte_len != j)
                            continue;
                        for (int l = j - 1; l >= 0; l--)
                        {
                            code = code << 8 | bytes[i + (j - 1 - l)];
                        }
                        if (code >= encoding->code_range_map[k].srcStart && code <= encoding->code_range_map[k].srcEnd)
                        {
                            cid = _convert_code_from_cmap(encoding, code);
                            printf("code = %5d cid = %5d gid = %5d ", code, cid, cid);
                            i += j;
                            found = true;
                            break;
                        }
                        else
                        {
                            code = 0;
                        }
                    }
                    if (found) break;
                }
                

                unicode[unicode_cnt].utf32 = cid;
                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF32;
                unicode[unicode_cnt].isGid = true;
                unicode_cnt++;
                
                if (to_unicode_map != NULL)
                {
                    for (int j = 0; j < to_unicode_map->code_range_map.size(); j++)
                    {
                        if (code >= to_unicode_map->code_range_map[j].srcStart && code <= to_unicode_map->code_range_map[j].srcEnd)
                        {
                            uint32_t uni = _convert_unicode_from_cmap(to_unicode_map, code);
                            wchar_t wc = uni;
                            printf("unicode = %5d (%lc)\n", code, uni, wc);
                            break;
                        }
                    }
                }
                else if (encoding && encoding->name && !strncmp(encoding->name, "Uni", 3))
                {
                    wchar_t wc = code;
                    printf("unicode = %5d(%lc)\n", code, wc);
                }
                else
                {
                    printf("\n");
                }
            }
        }
        else
        {
            for (int i = 0; i < bytes.size(); )
            {
                uint32_t code = 0;
                uint32_t cid = 0;
                for (int j = 4; j >= 1; j /= 2)
                {
                    bool found = false;
                    for (int k = 0; k < encoding->code_range_map.size(); k++)
                    {
                        if (encoding->code_range_map[k].byte_len != j)
                            continue;
                        for (int l = j - 1; l >= 0; l--)
                        {
                            code = code << 8 | bytes[i + (j - 1 - l)];
                        }
                        if (code >= encoding->code_range_map[k].srcStart && code <= encoding->code_range_map[k].srcEnd)
                        {
                            cid = _convert_code_from_cmap(encoding, code);
                            printf("code = %5d cid = %5d ", code, cid);
                            i += j;
                            found = true;
                            break;
                        }
                        else
                        {
                            code = 0;
                        }
                    }
                    if (found) break;
                }
                if (cid_to_gid_map != NULL)
                {
                    for (int j = 0; j < cid_to_gid_map->code_range_map.size(); j++)
                    {
                        if (cid >= cid_to_gid_map->code_range_map[j].srcStart && cid <= cid_to_gid_map->code_range_map[j].srcEnd)
                        {
                            uint32_t gid = _convert_code_from_cmap(cid_to_gid_map, cid);
                            printf("gid = %5d ", gid);
                            unicode[unicode_cnt].utf32 = gid;
                            unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF32;
                            unicode[unicode_cnt].isGid = true;
                            unicode_cnt++;
                            break;
                        }
                    }
                }
                else
                {
                    char to_unicode_name[64] = {0};
                    sprintf(to_unicode_name, "Adobe-%s-UCS2", cidfont->cid_system_info.ordering);
                    pdf_cmap_t* c = pdf_file_get_cmap(context->pdf, to_unicode_name);
                    if (c != NULL)
                    {
                        for (int j = 0; j < c->code_range_map.size(); j++)
                        {
                            if (cid >= c->code_range_map[j].srcStart && cid <= c->code_range_map[j].srcEnd)
                            {
                                uint32_t uni = _convert_unicode_from_cmap(c, cid);
                                wchar_t wc = uni;
                                printf("unicode = %5d (%lc)", uni, wc);

                                unicode[unicode_cnt].utf32 = uni;
                                unicode[unicode_cnt].encoding = PLUTOVG_TEXT_ENCODING_UTF32;
                                unicode[unicode_cnt].isGid = false;
                                unicode_cnt++;
                                break;
                            }
                        }
                    }
                }
                if (to_unicode_map != NULL)
                {
                    for (int j = 0; j < to_unicode_map->code_range_map.size(); j++)
                    {
                        if (code >= to_unicode_map->code_range_map[j].srcStart && code <= to_unicode_map->code_range_map[j].srcEnd)
                        {
                            uint32_t uni = _convert_unicode_from_cmap(to_unicode_map, code);
                            wchar_t wc = uni;
                            printf("unicode = %5d (%lc)\n", uni, wc);
                            break;
                        }
                    }
                }
                else
                {
                    printf("\n");
                }
            }
        }
    }

    if (unicode_cnt == 0)
    {
        plutovg_canvas_restore(context->canvas);
        return;
    }

    if (context->state->textState.font->subtype != FONT_SUBTYPE_TYPE3)
    {
        if (context->state->textState.fontface != NULL)
        {
            float x, y;
            plutovg_canvas_get_current_point(context->canvas, &x, &y);
            // cos(theta), sin(theta),  0
            // -sin(theta), cos(theta), 0
            // 0, 0, 1

            // 1,  0, 0
            // 0, -1, 0,
            // 0,  0, 1
            // rotate 180閹�?
            // or scale by 1
            plutovg_canvas_transform(context->canvas, &context->state->textState.textMatrix);
            plutovg_canvas_scale(context->canvas, 1, -1);
            plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
            plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
            plutovg_canvas_set_line_width(context->canvas, context->state->lineWidth);
            plutovg_canvas_set_miter_limit(context->canvas, context->state->miterLimit);
            plutovg_canvas_set_line_cap(context->canvas, (plutovg_line_cap_t)context->state->lineCap);
            plutovg_canvas_set_line_join(context->canvas, (plutovg_line_join_t)context->state->lineJoin);

            plutovg_canvas_new_path(context->canvas);
            float advance_width = 0;
            for (int i = 0; i < unicode_cnt; i++)
            {
                if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF8)
                {
                    advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf8, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0, unicode[i].isGid);
                }
                else if (unicode[i].encoding == PLUTOVG_TEXT_ENCODING_UTF16)
                {
                    advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf16, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0, unicode[i].isGid);
                }
                else
                {
                    advance_width = plutovg_canvas_add_text1(context->canvas, &unicode[i].utf32, 1, unicode[i].encoding, context->state->textState.textLineWidth, 0, unicode[i].isGid);
                }
                context->state->textState.textLineWidth += advance_width;
            }

             

            // TODO: text rendering mode support
            switch (context->state->textState.textMode)
            {
                case 0:// fill
                case 4:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->fill.color[0],
                    context->state->fill.color[1],
                    context->state->fill.color[2]);
                    plutovg_canvas_fill(context->canvas);
                    break;
                case 1: // stroke
                case 5:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->stroke.color[0],
                    context->state->stroke.color[1],
                    context->state->stroke.color[2]);
                    plutovg_canvas_stroke(context->canvas);
                    break;
                case 2: // fill and then stroke
                case 6:
                    plutovg_canvas_set_rgb(context->canvas,
                    context->state->stroke.color[0],
                    context->state->stroke.color[1],
                    context->state->stroke.color[2]);
                    plutovg_canvas_fill(context->canvas);
                    // plutovg_canvas_set_rgb(context->canvas,
                    // context->state->stroke.color[0],
                    // context->state->stroke.color[1],
                    // context->state->stroke.color[2]);
                    // plutovg_canvas_stroke(context->canvas);
                    break;
                // case 3: // invisible
                //     break;
                // case 4:
                //     plutovg_canvas_set_rgb(context->canvas,
                //     context->state->fill.color[0],
                //     context->state->fill.color[1],
                //     context->state->fill.color[2]);
                //     plutovg_canvas_fill(context->canvas);
                //     plutovg_canvas_clip_preserve(context->canvas);
                //     break;
                // case 5:
                //     plutovg_canvas_set_rgb(context->canvas,
                //     context->state->stroke.color[0],
                //     context->state->stroke.color[1],
                //     context->state->stroke.color[2]);
                //     plutovg_canvas_stroke(context->canvas);
                //     plutovg_canvas_clip_preserve(context->canvas);
                //     break;
                // case 6:
                //     plutovg_canvas_fill_preserve(context->canvas);
                //     plutovg_canvas_stroke(context->canvas);
                //     plutovg_canvas_new_path(context->canvas);
                //     break;
                // case 7:
                //     plutovg_canvas_clip_preserve(context->canvas);
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
            pdf_array_t* charstrings_index = font_descriptor->charstrings;
            pdf_array_t* font_dict_aar = font_descriptor->font_dict_arr;
            pdf_array_t* font_dict_select = font_descriptor->font_dict_select_arr;
            if (charstrings_index != NULL)
            {
                pdf_array_t* font_matrix = font_descriptor->font_matrix;
                plutovg_matrix_t original_matrix = context->state->textState.textMatrix;
                for (int i = 0; i < unicode_cnt; i++)
                {
                    pdf_deque_t* deque = pdf_deque_init();
                    plutovg_canvas_save(context->canvas);
                    plutovg_matrix_t font_matrix_plutovg, rm;
                    plutovg_matrix_init(&font_matrix_plutovg, 
                        font_matrix->get(0)->val.number, font_matrix->get(1)->val.number,
                        font_matrix->get(2)->val.number, font_matrix->get(3)->val.number,
                        font_matrix->get(4)->val.number, font_matrix->get(5)->val.number);
                    plutovg_matrix_multiply(&rm, &font_matrix_plutovg, &context->state->textState.textMatrix);
                    plutovg_matrix_t m = {
                        context->state->textState.fontSize * context->state->textState.horizontalScaling / 100, 0,
                        0, context->state->textState.fontSize,
                        0, context->state->textState.textRise
                    };
                    plutovg_matrix_multiply(&rm, &m, &rm);
                    plutovg_canvas_transform(context->canvas, &rm);
                    plutovg_canvas_set_rgb(context->canvas,
                        context->state->fill.color[0],
                        context->state->fill.color[1],
                        context->state->fill.color[2]);
                    plutovg_canvas_new_path(context->canvas);
                    pdf_cff_char_render_t ctx;
                    ctx.buf = (unsigned char*)charstrings_index->get(unicode[i].utf32)->val.string;
                    ctx.cur = ctx.buf;
                    ctx.len = charstrings_index->get(unicode[i].utf32)->value_len;
                    ctx.canvas = context->canvas;
                    ctx.fontSize = context->state->textState.fontSize;
                    ctx.global_bias = font_descriptor->global_subr_bias;
                    ctx.global_subr = font_descriptor->global_subr;
                    ctx.open = false;
                    ctx.havewidth = false;
                    ctx.width = 0;
                    ctx.curX = 0;
                    ctx.curY = 0;
                    ctx.stems = 0;
                    ctx.stemshm = 0;
                    ctx.fisr_stack_clear = true;
                    _cff_do_render_char(&ctx, deque);
                    pdf_dict_t* font_dict = NULL;
                    double defaultWidthX = 0;
                    double nominalWidthX = 0;
                    if ((int)(font_dict_select->get(0)->val.number) == 0)
                    {
                        int fd = font_dict_select->get(unicode[i].utf32 + 1)->val.number;
                        font_dict = font_dict_aar->get(fd)->val.dict;
                    }
                    else
                    {
                        for (int j = 1; j < font_dict_select->size(); j += 3)
                        {
                            if (unicode[i].utf32 >= (int)(font_dict_select->get(j)->val.number)
                            && unicode[i].utf32 <= (int)(font_dict_select->get(j + 1)->val.number))
                            {
                                int fd = (int)(font_dict_select->get(j + 2)->val.number);
                                font_dict = font_dict_aar->get(fd)->val.dict;
                                break;
                            }
                        }
                    }
                    if (font_dict != NULL)
                    {
                        defaultWidthX = pdf_dict_get_number(font_dict, "defaultWidthX");
                        nominalWidthX = pdf_dict_get_number(font_dict, "nominalWidthX");
                    }
                    double advance = defaultWidthX;
                    if ((int)(ctx.width) != 0)
                    {
                        advance = (ctx.width + nominalWidthX) * context->state->textState.fontSize / 1000.0;
                    }

                    context->state->textState.textLineWidth += advance;
                    plutovg_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                    plutovg_canvas_fill(context->canvas);
                    plutovg_canvas_restore(context->canvas); 
                    pdf_deque_free(deque);
                }
                context->state->textState.textMatrix = original_matrix;
                //plutovg_surface_write_to_png(context->surface, "test.png");
            }
        }
    }
    else
    {
        pdf_font_type3_t* type3 = context->state->textState.font->type3;
        float x, y;
        plutovg_canvas_get_current_point(context->canvas, &x, &y);
        // render Type3 font
        pdf_array_t* differences = type3->differences;
        if (differences != NULL)
        {
            // pdf_array_t* font_bbox = type3->font_bbox;
            pdf_array_t* font_matrix = type3->font_matrix;
            pdf_array_t* widths = type3->widths;
            // plutovg_canvas_scale(context->canvas, 1, -1);
            
            //plutovg_canvas_set_font_size(context->canvas, context->state->textState.fontSize);
            //plutovg_canvas_set_font_face(context->canvas, context->state->textState.fontface);
            plutovg_matrix_t original_matrix = context->state->textState.textMatrix;
            for (int i = 0; i < unicode_cnt; i++) 
            {
                uint32_t c = unicode[i].utf32;
                for (int j = 0; j < differences->size(); j += 2) 
                {
                    if ((uint32_t)differences->get(j)->val.number == c) 
                    {
                        const char* name = differences->get(j + 1)->val.name;
                        int ref = pdf_dict_get_ref(type3->charProcs, name);
                        if (ref != -1) 
                        {
                            pdf_obj_t* obj = pdf_file_get_obj(context->pdf, ref);
                            if (obj != NULL && obj->stream != NULL) 
                            {
                                pdf_obj_t* save_obj = context->current_obj;
                                context->current_obj = obj;

                                plutovg_canvas_save(context->canvas);
                                plutovg_matrix_t font_matrix_plutovg, rm;
                                plutovg_matrix_init(&font_matrix_plutovg, 
                                    font_matrix->get(0)->val.number, font_matrix->get(1)->val.number,
                                    font_matrix->get(2)->val.number, font_matrix->get(3)->val.number,
                                    font_matrix->get(4)->val.number, font_matrix->get(5)->val.number);
                                plutovg_matrix_multiply(&rm, &font_matrix_plutovg, &context->state->textState.textMatrix);
                                plutovg_matrix_t m = {
                                    context->state->textState.fontSize * context->state->textState.horizontalScaling / 100, 0,
                                    0, context->state->textState.fontSize,
                                    0, context->state->textState.textRise
                                };
                                plutovg_matrix_multiply(&rm, &m, &rm);
                                plutovg_canvas_transform(context->canvas, &rm);
                                if (widths != NULL)
                                {
                                    float width = 0;
                                    if (c >= type3->first_char && c <= type3->last_char)
                                    {
                                        width = widths->get(c - type3->first_char)->val.number;
                                    }
                                    // plutovg_canvas_translate(context->canvas, x + context->state->textState.textLineWidth, 0);
                                    double advance = width * context->state->textState.fontSize / 1000.0;
                                    context->state->textState.textLineWidth += advance;
                                    plutovg_matrix_translate(&context->state->textState.textMatrix, advance, 0);
                                }
                                plutovg_canvas_set_rgb(context->canvas,
                                    context->state->fill.color[0],
                                    context->state->fill.color[1],
                                    context->state->fill.color[2]);
                                // if (font_bbox != NULL) 
                                // {
                                //     //plutovg_canvas_new_path(context->canvas);
                                //     plutovg_canvas_rect(context->canvas,
                                //         font_bbox->values[0]->val.number,
                                //         font_bbox->values[1]->val.number,
                                //         font_bbox->values[2]->val.number,
                                //         font_bbox->values[3]->val.number);
                                //     plutovg_canvas_clip(context->canvas);
                                // }
                                plutovg_canvas_new_path(context->canvas);
                                pdf_stream_open(obj->stream);
                                pdf_token_t* tk = NULL;
                                while ((tk = pdf_stream_get_next_token(obj->stream)) != NULL) 
                                {
                                    if (tk->type() == TOKEN_STREAM_END)
                                        break;
                                    _do_render_operation(obj->stream, context, tk);
                                    delete tk;
                                }
                                pdf_stream_close(obj->stream);
                                context->current_obj = save_obj;
                                
                                plutovg_canvas_restore(context->canvas);
                            }
                        }
                        break;
                    }
                }
            }
            context->state->textState.textMatrix = original_matrix;
        }
    }
    plutovg_canvas_restore(context->canvas);
}
