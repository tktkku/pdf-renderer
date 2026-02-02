#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
const static char* TOKEN_NAMES[] = {
    #define TOKEN_DEF(v, t) v,
    #include "pdf-token.def"
    };
const char space_tag[] = {
    // 0 9 10 12 13 32
    '\0', '\t', '\n', '\f', '\r', ' '
};
bool _is_space(char c)
{
    int len = ARRAY_COUNT(space_tag);
    int left = 0;
    int right = len - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        if (c < space_tag[mid])
        {
            right = mid - 1;
        }
        else if (c > space_tag[mid])
        {
            left = mid + 1;
        }
        else
        {
            return true;
        }
    }

    return false;
}
bool _is_digit(char c)
{
    return (c >= '0' && c <= '9');
}
bool _is_hex(char c)
{
    return
        _is_digit(c)
        || ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}
const char delimiter_tag[] = {
    // 37 40 41 47 60 62 91 93 123 125
    '%', '(', ')', '/', '<', '>', '[', ']', '{', '}',
};
bool _is_delimiter(char c)
{
    int len = ARRAY_COUNT(delimiter_tag);
    int left = 0;
    int right = len - 1;
    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        if (c < delimiter_tag[mid])
        {
            right = mid - 1;
        }
        else if (c > delimiter_tag[mid])
        {
            left = mid + 1;
        }
        else
        {
            return true;
        }
    }

    return false;
}

typedef struct
{
    int nums;
    pdf_parser_token_type_t tokens[40];
} fixed_token_map_t;
#define MAX_FIXED_TOKEN_LEN 19
const static fixed_token_map_t fixed_token_map[MAX_FIXED_TOKEN_LEN + 1] =
{
    {0},
    //1
    {29, {
        TOKEN_OPERATOR_quotation, // 34 
        TOKEN_OPERATOR_apostrophe, // 39
        TOKEN_OPERATOR_B,// 66
        TOKEN_OPERATOR_F, // 70
        TOKEN_OPERATOR_G, // 71
        TOKEN_OPERATOR_J, // 74
        TOKEN_OPERATOR_K, // 75
        TOKEN_OPERATOR_M, // 77
        TOKEN_OPERATOR_Q, // 81
        TOKEN_INDIRECT, // 82
        TOKEN_OPERATOR_S, // 83
        TOKEN_OPERATOR_W, // 87
        //{"[", TOKEN_ARRAY_BEG, 1}, // 91
        //{"[", TOKEN_ARRAY_END, 1}, // 93
        TOKEN_OPERATOR_b, //98
        TOKEN_OPERATOR_c, // 99
        TOKEN_OPERATOR_d, // 100
        TOKEN_OPERATOR_f, // 102
        TOKEN_OPERATOR_g,  //103
        TOKEN_OPERATOR_h, // 104
        TOKEN_OPERATOR_i, // 105
        TOKEN_OPERATOR_j, // 106
        TOKEN_OPERATOR_k, // 107
        TOKEN_OPERATOR_l, // 108
        TOKEN_OPERATOR_m, //109
        TOKEN_OPERATOR_n, // 110
        TOKEN_OPERATOR_q, // 113
        TOKEN_OPERATOR_s, // 115
        TOKEN_OPERATOR_v, // 118
        TOKEN_OPERATOR_w, // 119
        TOKEN_OPERATOR_y // 121
    }},
    // 2
    {38, {
        //{"<<", TOKEN_DICT_BEG, 2}, // 60
        //{">>", TOKEN_DICT_END, 2}, // 62
        TOKEN_OPERATOR_B_star,
        TOKEN_OPERATOR_BI,
        TOKEN_OPERATOR_BT,
        TOKEN_OPERATOR_CS,
        TOKEN_OPERATOR_DP,
        TOKEN_OPERATOR_Do,
        TOKEN_OPERATOR_EI,
        TOKEN_OPERATOR_ET,
        TOKEN_OPERATOR_ID,
        TOKEN_OPERATOR_MP,
        TOKEN_OPERATOR_RG,
        TOKEN_OPERATOR_SC,
        TOKEN_OPERATOR_T_star,
        TOKEN_OPERATOR_TD,
        TOKEN_OPERATOR_TJ,
        TOKEN_OPERATOR_TL,
        TOKEN_OPERATOR_Tc,
        TOKEN_OPERATOR_Td,
        TOKEN_OPERATOR_Tf,
        TOKEN_OPERATOR_Tj,
        TOKEN_OPERATOR_Tm,
        TOKEN_OPERATOR_Tr,
        TOKEN_OPERATOR_Ts,
        TOKEN_OPERATOR_Tw,
        TOKEN_OPERATOR_Tz,
        TOKEN_OPERATOR_W_star,
        TOKEN_OPERATOR_b_star,
        TOKEN_OPERATOR_cm,
        TOKEN_OPERATOR_cs,
        TOKEN_OPERATOR_d0,
        TOKEN_OPERATOR_d1,
        TOKEN_OPERATOR_f_star,
        TOKEN_OPERATOR_gs,
        TOKEN_OPERATOR_re,
        TOKEN_OPERATOR_rg,
        TOKEN_OPERATOR_ri,
        TOKEN_OPERATOR_sc,
        TOKEN_OPERATOR_sh,
    }},
    // 3
    {9, {
        TOKEN_OPERATOR_BDC,
        TOKEN_OPERATOR_BMC,
        TOKEN_OPERATOR_EMC,
        TOKEN_OPERATOR_SCN,
        TOKEN_DEF,
        TOKEN_DUP,
        TOKEN_END,
        TOKEN_OBJ_BEG,
        TOKEN_OPERATOR_scn
    }},
    // 4
    {4, {
        TOKEN_DICT,
        TOKEN_NULL,
        TOKEN_BOOLEAN_TRUE,
        TOKEN_XREF,
    }},
    // 5
    {2,{
        TOKEN_BEGIN,
        TOKEN_BOOLEAN_FALSE
    }},
    // 6
    {2, {
        TOKEN_OBJ_END,
        TOKEN_STREAM_BEG,
    }},
    // 7
    {1, {
        TOKEN_ENDCMAP,
    }},
    // 8
    {0},
    // 9
    {3, {
        TOKEN_BEGINCMAP,
        TOKEN_ENDBFCHAR,
        TOKEN_STREAM_END,
    }},
    // 10
    {2, {
        TOKEN_ENDBFRANGE,
        TOKEN_ENDCIDCHAR,
    }},
    // 11
    {2, {
        TOKEN_BEGINBFCHAR,
        TOKEN_ENDCIDRANGE,
    }},
    // 12
    {3, {
        TOKEN_BEGINBFRANGE,
        TOKEN_BEGINCIDCHAR,
        TOKEN_FINDRESOURCE,
    }},
    // 13
    {1, {
        TOKEN_BEGINCIDRANGE,
    }},
    // 14
    {1, {
        TOKEN_ENDNOTDEFRANGE
    }}, 
    // 15
    {0}, 
    // 16
    {1, {
        TOKEN_BEGINNOTDEFRANGE
    }},
    // 17
    {1, {
        TOKEN_ENDCODESPACERANGE,
    }}, 
    // 18
    {0},
    // 19
    {1, {
        TOKEN_BEGINCODESPACERANGE,
    }}
};
const char* _token_to_string(pdf_parser_token_type_t type)
{
    return TOKEN_NAMES[type];
}

pdf_parser_token_t* _parse_number(pdf_parser_t* parser, const unsigned char* start, const unsigned char* end)
{
    int len = 1;
    const unsigned char* p = start;

    char c = *p;
    bool has_dot = false;
    if (_is_digit(c) || c == '.')
    {
    process_number:
        if (c == '.')
            has_dot = true;

        while (p < end)
        {
            p++; len++;
            c = *p;
            if (_is_digit(c))
            {
                continue;
            }
            else if (c == '.')
            {
                if (!has_dot)
                {
                    has_dot = true;
                }
                else
                {
                    //pdf_parser_error("error number: %c", c);
                    return NULL;
                }
            }
            else
            {
                p--; len--;
                break;
            }
        }
        pdf_parser_token_t* tk = pdf_parser_token_init(parser, start, TOKEN_NUMBER, len);

        return tk;
    }
    else if (c == '-' || c == '+')
    {
        if (p < end)
        {
            p++; len++;
            c = *p;
            if (_is_digit(c) || c == '.')
            {
                goto process_number;
            }
            else
            {
                //pdf_parser_error("unknown token: %c(%#x)", c, c);
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        //pdf_parser_error("unknown token: %c(%#x)", c, c);
        return NULL;
    }
}

pdf_parser_token_t* pdf_parser_token_init(pdf_parser_t* parser, const unsigned char* start, pdf_parser_token_type_t type, int len)
{
    if (parser == NULL || start == NULL || len == 0) return NULL;
    pdf_parser_token_t* tk = NULL;
    // if (len <= MAX_FIXED_TOKEN_LEN)
    // {
    //     if (parser->pdf->freed_tokens != NULL)
    //     {
    //         tk = parser->pdf->freed_tokens;
    //         parser->pdf->freed_tokens = parser->pdf->freed_tokens->next;
    //     }
    //     else
    //     {
    //         tk = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
    //         tk->token = malloc(MAX_FIXED_TOKEN_LEN + 1);
    //     }
    // }
    // else
    {
        tk = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
        tk->token = (char*)malloc(len + 1);
    }
    tk->type = type;
    memcpy(tk->token, start, len);
    tk->token[len] = '\0';
    tk->token_len = len;
    tk->steps = len;
    tk->next = NULL;

    return tk;
}
void pdf_parser_token_free(pdf_parser_t* parser, pdf_parser_token_t* token)
{
    if (token == NULL) return;
    // if (token->token_len <= MAX_FIXED_TOKEN_LEN && parser != NULL)
    // {
    //     token->next = parser->pdf->freed_tokens;
    //     parser->pdf->freed_tokens = token;
    // }
    // else
    {
        if (token->token)
        {
            free(token->token);
            token->token = NULL;
        }

        free(token);
        token = NULL;
    }
}

const char* pdf_parser_token_get_token(pdf_parser_token_t* token)
{
    if (token == NULL) return NULL;

    return token->token;
}
pdf_parser_t* pdf_parser_init(pdf_file_t* pdf, input_t* input)
{
    // if (pdf == NULL) return NULL;
    pdf_parser_t* parser = (pdf_parser_t*)calloc(1, sizeof(pdf_parser_t));
    parser->pdf = pdf;
    parser->buffer = (unsigned char*)malloc(4096);
    parser->buffer_size = 4096;
    parser->splite_pos = NULL;
    parser->current_pos = NULL;
    parser->end_pos = NULL;
    parser->remain.rem = NULL;
    parser->remain.len = 0;
    parser->num_cached_tokens = 0;
    parser->input = input;
    parser->pause_read = false;
    parser->eof = false;

    return parser;
}
void pdf_parser_free(pdf_parser_t* parser)
{
    if (parser->buffer)
    {
        free(parser->buffer);
        parser->buffer = NULL;
    }
    int remain = parser->end_pos - parser->current_pos;
    if (remain > 0)
    {
        input_seek(parser->pdf->input, -remain, SEEK_CUR);
    }
    if (parser->remain.len > 0)
    {
        free(parser->remain.rem);
    }
    if (parser->num_cached_tokens > 0)
    {
        for (int i = 0; i < parser->num_cached_tokens; i++)
        {
            pdf_parser_token_free(parser, parser->cached_tokens[i]);
            parser->cached_tokens[i] = NULL;
        }
    }
    free(parser);
    parser = NULL;
}


size_t pdf_parser_read_data(pdf_parser_t* parser, void* ptr, size_t size)
{
    if (parser == NULL || ptr == NULL || size == 0)
        return 0;

    unsigned char* out_ptr = (unsigned char*)ptr;
    size_t bytes_read = 0;
    size_t bytes_remaining = size;

    while (bytes_remaining > 0)
    {
        if (parser->current_pos != NULL && parser->splite_pos != NULL)
        {
            size_t available = parser->splite_pos - parser->current_pos + 1;
            if (available > 0)
            {
                size_t copy_size = (bytes_remaining < available) ? bytes_remaining : available;
                memcpy(out_ptr + bytes_read, parser->current_pos, copy_size);
                parser->current_pos += copy_size;
                bytes_read += copy_size;
                bytes_remaining -= copy_size;
                continue;
            }
        }

        if (parser->eof)
        {
            break;
        }

        _pdf_parser_read_input(parser);
    }

    return bytes_read;
}
void _decode_hex_string(char* str, int len, int* out_len)
{
    char* p = str + 1; // skip '<'
    char* end = str + len;
    char* out = (char*)malloc(len);
    int ol = 0;
    out[ol++] = *str;
    while (p < end)
    {
        if (_is_hex(*p))
            out[ol++] = *p;

        p++;
    }
    memcpy(str, out, ol);
    str[ol] = '\0';
    *out_len = ol;
    free(out);
}
void _decode_name(char* str, int len, int* out_len)
{
    char* p = str + 1; // skip '/'
    char* end = str + len;
    char* out = (char*)malloc(len);
    int ol = 0;
    out[ol++] = *str; //'/'
    while (p < end)
    {
        if (*p == '#')
        {
            if (p + 1 >= end)
            {
                out[ol++] = *p;
                break;
            }
            p++;
            uint8_t c = _hex_str_to_8bit(p); p += 2;
            out[ol++] = c;
        }
        else
        {
            out[ol++] = *p;
            p++;
        }
    }
    memcpy(str, out, ol);
    str[ol] = '\0';
    *out_len = ol;
    free(out);
}
void _decode_string(char* str, int len, int* out_len)
{
    char* p = str + 1; // skip '('
    char* end = str + len;
    char* out = (char*)malloc(len);
    int ol = 0;
    out[ol++] = *str; //'('
    while (p < end)
    {
        if (*p == '\\')
        {
            if (p + 1 >= end)
            {
                out[ol++] = *p;
                break;
            }
            p++;
            if (*p == 'n')
            {
                out[ol++] = '\n';
            }
            else if (*p == '\n')
            {
                // ignored
            }
            else if (*p == 'r')
            {
                out[ol++] = '\r';
            }
            else if (*p == 't')
            {
                out[ol++] = '\t';
            }
            else if (*p == 'b')
            {
                out[ol++] = '\b';
            }
            else if (*p == 'f')
            {
                out[ol++] = '\f';
            }
            else if (*p == '(' || *p == ')' || *p == '\\')
            {
                out[ol++] = *p;
            }
            else if (_is_digit(*p))
            {
                char octal[3] = { 0 };
                for (int i = 0; i < 3 && p < end; i++, p++)
                {
                    if (_is_digit(*p))
                    {
                        octal[i] = *p;
                    }
                    else
                    {
                        break;
                    }
                }

                char c = (char)strtol(octal, NULL, 8);
                out[ol++] = c;
            }
        }
        else
        {
            out[ol++] = *p;
        }
        p++;
    }
    memcpy(str, out, ol);
    str[ol] = '\0';
    *out_len = ol;
    free(out);
}

pdf_parser_token_t* _pdf_parser_next_one_token(pdf_parser_t* parser, const unsigned char* start, const unsigned char* end)
{
    if (start == NULL)
        return NULL;
    pdf_parser_token_t* tk = NULL;
    unsigned char c = *start;
    do
    {
        int len = 1;
        const unsigned char* p = start;
        if (c == '-' || c == '+' || c == '.' || _is_digit(c))
        {
            goto PARSE_NUMBER;
        }
        else if (_is_space(c))
        {
            goto PARSE_SPACE;
        }
        if (_is_delimiter(*p))
        {
            goto NOT_OPERATOR;
        }
        while (p < end)
        {
            p++; len++;
            c = *p;
            if (_is_space(c) || _is_delimiter(*p))
            {
                p--; len--;
                break;
            }
        }
        if (len > MAX_FIXED_TOKEN_LEN)
            return NULL;

        const pdf_parser_token_type_t* to_compare = fixed_token_map[len].tokens;
        int operator_count = fixed_token_map[len].nums;
        bool isfind = false;
        int left = 0;
        int right = operator_count - 1;
        int mid = -1;
        while (left <= right)
        {
            mid = left + (right - left) / 2;
            int cmp = memcmp(start, TOKEN_NAMES[to_compare[mid]], len);
            if (cmp < 0)
            {
                right = mid - 1;
            }
            else if (cmp > 0)
            {
                left = mid + 1;
            }
            else
            {
                isfind = true;
                break;
            }
        }
        if (isfind)
        {
            //tk = pdf_parser_token_init(parser, start, to_compare[mid].type, len);
            tk = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
            tk->type = to_compare[mid];
            tk->next = NULL;
            tk->token_len = len;
            if (tk->type == TOKEN_NULL || tk->type == TOKEN_BOOLEAN_TRUE || tk->type == TOKEN_BOOLEAN_FALSE)
            {
                tk->token = (char*)malloc(len + 1);
                memcpy(tk->token, start, len);
                tk->token[len] = '\0';
            }
            else
                tk->token = NULL;
            
            tk->steps = len;
            start += len;
            return tk;
        }
        else
        {
            return NULL;
        }
    } while (0);
NOT_OPERATOR:
    c = *start;
    if (c == '/') // parse name
    {
        int len = 1;
        const unsigned char* p = start;
        while (p < end)
        {
            p++;len++;
            c = *p;
            if (_is_space(c) || _is_delimiter(c))
            {
                p--; len--;
                break;
            }
        }

        tk = pdf_parser_token_init(parser, start, TOKEN_NAME, len);
        start += len;
        // decode name
        _decode_name(tk->token, tk->token_len, &tk->token_len);
    }
    else if (c == '(') // parse string
    {
        int left_par = 1, right_par = 0;
        int len = 1;
        const unsigned char* p = start;
        while (p < end)
        {
            p++; len++;
            c = *p;
            if (c == '\\')
            {
                // skip the next char
                p++; len++;
                continue;
            }
            else if (c == '(')
            {
                left_par++;
            }
            else if (c == ')')
            {
                right_par++;
                if (left_par == right_par)
                {
                    break;
                }
            }
        }
        if (left_par != right_par)
        {
            return NULL;
        }
        tk = pdf_parser_token_init(parser, start, TOKEN_STRING, len);
        tk->token[tk->token_len - 1] = '\0';
        tk->token_len--;
        start += len;
        // decode string
        _decode_string(tk->token, tk->token_len, &tk->token_len);
    }
    else if (c == '[')
    {
        int len = 1;
        tk = pdf_parser_token_init(parser, start, TOKEN_ARRAY_BEG, len);
        start += len;
    }
    else if (c == ']')
    {
        int len = 1;
        tk = pdf_parser_token_init(parser, start, TOKEN_ARRAY_END, len);
        start += len;

    }
    else if (c == '<')
    {
        if (memcmp(start, "<<", 2) == 0) // dict
        {
            int len = 2;
            tk = pdf_parser_token_init(parser, start, TOKEN_DICT_BEG, len);
            start += len;

        }
        else
        {
            int len = 1;
            const unsigned char* p = start;
            while (p < end)
            {
                p++; len++;
                char c = *p;
                if (c == '>')
                {
                    break;
                }
            }

            tk = pdf_parser_token_init(parser, start, TOKEN_HEX_STRING, len);
            tk->token[tk->token_len - 1] = '\0';
            tk->token_len--;
            start += len;
            // decode hex string
            _decode_hex_string(tk->token, tk->token_len, &tk->token_len);
        }
    }
    else if (c == '>')
    {
        if (memcmp(start, ">>", 2) == 0) // dict
        {
            int len = 2;
            tk = pdf_parser_token_init(parser, start, TOKEN_DICT_END, len);
            start += len;

        }
        else
        {
            return NULL;
        }
    }
    else if (c == '%')
    {
        int len = 1;
        const unsigned char* p = start;
        while (p < end)
        {
            p++; len++;
            char c = *p;
            if (c == '\n')
            {
                p--; len--;
                break;
            }
        }
        tk = pdf_parser_token_init(parser, start, TOKEN_COMMENT, len);
        start += len;
    }
    else if (c == '-' || c == '+' || c == '.' || _is_digit(c))
    {
    PARSE_NUMBER:
        tk = _parse_number(parser, start, end);
        if (tk != NULL)
            start += tk->steps;
    }
    else if (_is_space(c))
    {
    PARSE_SPACE:
        if (memcmp(start, "\r\n", 2) == 0)
        {
            tk = pdf_parser_token_init(parser, start, TOKEN_NEWLINE, 2);
            start += tk->steps;
        }
        else
        {
            tk = pdf_parser_token_init(parser, start, TOKEN_SPACE, 1);
            start += tk->steps;
        }
    }
    else
    {
        //pdf_parser_error("unknow token: %c(%#x)", *start, *start);
        return NULL;
    }

    return tk;
}
int _pdf_parser_copy_rem(pdf_parser_t* parser)
{
    int off = 0;
    if (parser->splite_pos != NULL
        && parser->current_pos != NULL
        && parser->splite_pos >= parser->current_pos)
    {
        int len = parser->splite_pos - parser->current_pos + 1;
        memcpy(parser->buffer, parser->current_pos, len);
        off += len;
    }
    memset(parser->buffer + off, 0, parser->buffer_size - off);
    parser->current_pos = parser->buffer;

    if (parser->remain.len > 0)
    {
        memcpy(parser->buffer + off, parser->remain.rem, parser->remain.len);
        off += parser->remain.len;
        free(parser->remain.rem);
        parser->remain.rem = NULL;
        parser->remain.len = 0;
    }

    return off;
}
void _pdf_parser_split(pdf_parser_t* parser, int end_i)
{
    parser->end_pos = parser->buffer + end_i;

    while (end_i >= 0)
    {
        char c = parser->buffer[end_i];
        if (_is_delimiter(c) || _is_space(c))
        {
            parser->splite_pos = parser->buffer + end_i;

            int len = parser->end_pos - parser->splite_pos;
            if (len > 0)
            {
                parser->remain.rem = (unsigned char*)calloc(1, len + 1);
                parser->remain.len = len;
                memcpy(parser->remain.rem, parser->buffer + end_i + 1, len);
                parser->remain.rem[len] = '\0';
            }
            break;
        }
        end_i--;
    }
}
void _pdf_parser_read_input(pdf_parser_t* parser)
{
    if (parser == NULL)
    {
        return;
    }
    input_t* input = parser->input;
    int off = _pdf_parser_copy_rem(parser);
    int ret = 0;
    if (parser->buffer_size < off)
    {
        printf("off error in parser\n");
        return;
    }
    else if (parser->buffer_size > off)
    {
        ret = input_read(input, parser->buffer + off, parser->buffer_size - off);
        if (ret < 0)
        {
            return;
        }
        else if (ret == 0)
        {
            parser->end_pos = parser->buffer + off - 1;
            parser->splite_pos = parser->buffer + off;
            parser->eof = true;
        }
    }
        
    int end_i = off + ret - 1;
    _pdf_parser_split(parser, end_i);
    parser->pause_read = false;
}

pdf_parser_token_t* _pdf_next_token(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;
    unsigned char** start = &(parser->current_pos);
    unsigned char* end = parser->splite_pos;
    while (!parser->pause_read && *start < end && parser->num_cached_tokens < 3)
    {
        pdf_parser_token_t* tk = _pdf_parser_next_one_token(parser, *start, end);
        if (tk == NULL)
        {
            if (parser->input->type == INPUT_TYPE_STREAM)
            {
                if (parser->input->stream->processed < parser->input->stream->stream_len)
                {
                    _pdf_parser_read_input(parser);
                    tk = _pdf_parser_next_one_token(parser, *start, end);
                    if (tk == NULL) break;
                }
            }
            break;
        }

        if (tk->type == TOKEN_SPACE || tk->type == TOKEN_COMMENT || tk->type == TOKEN_NEWLINE)
        {
            // ignored
            *start += tk->steps;
            pdf_parser_token_free(parser, tk);
            continue;
        }
        else if (tk->type == TOKEN_STREAM_BEG)
        {
            parser->pause_read = true;
        }
        parser->cached_tokens[parser->num_cached_tokens++] = tk;
        *start += tk->steps;
    }
    if (parser->num_cached_tokens == 0)
        return NULL;
    pdf_parser_token_t* tk = parser->cached_tokens[0];
    parser->cached_tokens[0] = parser->cached_tokens[1];
    parser->cached_tokens[1] = parser->cached_tokens[2];
    parser->cached_tokens[2] = NULL;
    parser->num_cached_tokens--;
    if (parser->num_cached_tokens == 0)
    {
        return tk;
    }
    else if (tk->type == TOKEN_INDIRECT || tk->type == TOKEN_OBJ_BEG)
    {
        pdf_parser_token_free(parser, tk);
        return NULL;
    }
    else if (tk->type == TOKEN_NUMBER)
    {
        if (parser->num_cached_tokens < 2)
        {
            return tk;
        }
        if (parser->cached_tokens[0]->type != TOKEN_NUMBER)
        {
            return tk;
        }
        if (parser->cached_tokens[1]->type == TOKEN_OBJ_BEG
            || parser->cached_tokens[1]->type == TOKEN_INDIRECT)
        {
            pdf_parser_token_t* token = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
            token->type = parser->cached_tokens[1]->type;
            token->steps = tk->steps
                + parser->cached_tokens[0]->steps
                + parser->cached_tokens[1]->steps;
            token->token_len = tk->token_len
                + parser->cached_tokens[0]->token_len
                + parser->cached_tokens[1]->token_len
                + 2;// add 2 spaces
            if (token->token_len <= MAX_FIXED_TOKEN_LEN)
            {
                token->token = (char*)malloc(MAX_FIXED_TOKEN_LEN + 1);
            }
            else
            {
                token->token = (char*)malloc(token->token_len + 1);
            }
            int off = 0;
            memcpy(token->token, tk->token, tk->token_len);
            off += tk->token_len;
            token->token[off] = ' ';
            off++;
            pdf_parser_token_free(parser, tk);

            memcpy(token->token + off, parser->cached_tokens[0]->token, parser->cached_tokens[0]->token_len);
            off += parser->cached_tokens[0]->token_len;
            token->token[off] = ' ';
            off++;
            pdf_parser_token_free(parser, parser->cached_tokens[0]);

            // memcpy(token->token + off, parser->cached_tokens[1]->token, parser->cached_tokens[1]->token_len);
            // off += parser->cached_tokens[1]->token_len;
            token->token[off] = '\0';
            pdf_parser_token_free(parser, parser->cached_tokens[1]);
            parser->num_cached_tokens = 0;
            return token;
        }
    }
    return tk;
}
pdf_parser_token_t* pdf_stream_get_next_token(pdf_stream_t* stream)
{
    return pdf_parser_next_token(stream->parser);
}

pdf_parser_token_t* pdf_parser_next_token(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;

    // unsigned char* save_cur = parser->current_pos;
    if (parser->current_pos >= parser->splite_pos && !parser->eof)
    {
        _pdf_parser_read_input(parser);
    }
    pdf_parser_token_t* tk = _pdf_next_token(parser);
    // if (tk == NULL)
    // {
    //     if (parser->reader.type == STREAM_READER)
    //     {
    //         pdf_stream_t* s = (pdf_stream_t*)parser->reader.source;
    //         if (s->processed < s->stream_len)
    //         {
    //             parser->current_pos = save_cur;
    //             parser->reader.read(parser, parser->reader.source);
    //             tk = _pdf_next_token(parser);
    //         }
    //     }
    // }
    return tk;
}
pdf_cmap_t* pdf_parser_build_cmap(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;
    pdf_cmap_t* cmap = pdf_cmap_init();
    pdf_parser_token_t* last_token = NULL;
    pdf_parser_token_t* current_token = NULL;

    while ((current_token = pdf_parser_next_token(parser)) != NULL)
    {
        if (current_token->type == TOKEN_ENDCMAP)
        {
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
            pdf_parser_token_free(parser, current_token);
            current_token = NULL;
            break;
        }
        else if (current_token->type == TOKEN_BEGINBFCHAR || current_token->type == TOKEN_BEGINCIDCHAR)
        {
            // single char mapping
            bool isCid = (current_token->type == TOKEN_BEGINCIDCHAR);
            int unicode_map_len = strtol(last_token->token, NULL, 10);
            if (cmap->unicode_map == NULL)
            {
                cmap->unicode_map = (pdf_unicode_map_t*)malloc(sizeof(pdf_unicode_map_t) * unicode_map_len);
                if (cmap->unicode_map == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
            }
            else
            {
                pdf_unicode_map_t* t = (pdf_unicode_map_t*)realloc(cmap->unicode_map, sizeof(pdf_unicode_map_t) * (unicode_map_len + cmap->unicode_map_len));
                if (t == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                cmap->unicode_map = t;
            }
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
            pdf_parser_token_free(parser, current_token);
            current_token = NULL;

            for (int i = 0; i < unicode_map_len; i++)
            {
                pdf_unicode_map_t* unicode_map = cmap->unicode_map + i + cmap->unicode_map_len;
                last_token = pdf_parser_next_token(parser);
                current_token = pdf_parser_next_token(parser);
                unicode_map->cid = _hex_str_to_32bit(last_token->token + 1, last_token->token_len - 1);
                if (isCid)
                {
                    unicode_map->unicode = (uint32_t)strtol(current_token->token + 1, NULL, 10);
                }
                else
                {
                    unicode_map->unicode = _hex_str_to_32bit(current_token->token + 1, current_token->token_len - 1);
                }

                pdf_parser_token_free(parser, last_token);
                last_token = NULL;
                pdf_parser_token_free(parser, current_token);
                current_token = NULL;
            }
            cmap->unicode_map_len += unicode_map_len;
            continue;
        }
        else if (current_token->type == TOKEN_BEGINBFRANGE || current_token->type == TOKEN_BEGINCIDRANGE)
        {
            bool isCid = (current_token->type == TOKEN_BEGINCIDRANGE);
            int char_range_map_len = strtol(last_token->token, NULL, 10);
            if (cmap->char_range_map == NULL)
            {
                cmap->char_range_map = (pdf_char_range_map_t*)malloc(sizeof(pdf_char_range_map_t) * char_range_map_len);
                if (cmap->char_range_map == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
            }
            else
            {
                pdf_char_range_map_t* t = (pdf_char_range_map_t*)realloc(cmap->char_range_map, sizeof(pdf_char_range_map_t) * (char_range_map_len + cmap->char_range_map_len));
                if (t == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                cmap->char_range_map = t;
            }
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
            pdf_parser_token_free(parser, current_token);
            current_token = NULL;
            for (int i = 0; i < char_range_map_len; i++)
            {
                pdf_char_range_map_t* char_range_map = cmap->char_range_map + i + cmap->char_range_map_len;
                pdf_parser_token_t* tk1 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk2 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk3 = pdf_parser_next_token(parser);
                char_range_map->srcStart = _hex_str_to_32bit(tk1->token + 1, tk1->token_len - 1);
                char_range_map->srcEnd = _hex_str_to_32bit(tk2->token + 1, tk2->token_len - 1);
                if (isCid)
                {
                    char_range_map->dstStart = (uint32_t)strtol(tk3->token, NULL, 10);
                }
                else
                {
                    if (tk3->type != TOKEN_ARRAY_BEG)
                    {
                        char_range_map->dstStart = _hex_str_to_32bit(tk3->token + 1, tk3->token_len - 1);
                    }
                    else
                    {
                        pdf_array_t* arr = pdf_parser_build_array(parser);
                        for (int i = 0; i < arr->num_elements; i++)
                        {
                            pdf_char_range_map_t* char_range_map1 = (pdf_char_range_map_t*)malloc(sizeof(pdf_char_range_map_t));
                            char_range_map1->srcStart = char_range_map->srcStart + i;
                            char_range_map1->srcEnd = char_range_map->srcStart + i;
                            char_range_map1->dstStart = _hex_str_to_32bit(arr->values[i]->val.string + 1, arr->values[i]->value_len - 1);
                        }
                        pdf_array_free(arr);
                        free(char_range_map);
                    }
                }
                pdf_parser_token_free(parser, tk1);
                pdf_parser_token_free(parser, tk2);
                pdf_parser_token_free(parser, tk3);
            }
            cmap->char_range_map_len += char_range_map_len;
            continue;
        }
        else if (current_token->type == TOKEN_BEGINNOTDEFRANGE)
        {
            int char_range_map_len = strtol(last_token->token, NULL, 10);
            if (cmap->not_def_range == NULL)
            {
                cmap->not_def_range = (pdf_char_range_map_t*)malloc(sizeof(pdf_char_range_map_t) * char_range_map_len);
                if (cmap->not_def_range == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
            }
            else
            {
                pdf_char_range_map_t* t = (pdf_char_range_map_t*)realloc(cmap->not_def_range, sizeof(pdf_char_range_map_t) * (char_range_map_len + cmap->not_def_range_len));
                if (t == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                cmap->not_def_range = t;
            }
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
            pdf_parser_token_free(parser, current_token);
            current_token = NULL;
            for (int i = 0; i < char_range_map_len; i++)
            {
                pdf_char_range_map_t* char_range_map = cmap->not_def_range + i + cmap->not_def_range_len;
                pdf_parser_token_t* tk1 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk2 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk3 = pdf_parser_next_token(parser);
                char_range_map->srcStart = _hex_str_to_32bit(tk1->token + 1, tk1->token_len - 1);
                char_range_map->srcEnd = _hex_str_to_32bit(tk2->token + 1, tk2->token_len - 1);
                char_range_map->dstStart = (uint32_t)strtol(tk3->token, NULL, 10);
                pdf_parser_token_free(parser, tk1);
                pdf_parser_token_free(parser, tk2);
                pdf_parser_token_free(parser, tk3);
            }
            cmap->not_def_range_len += char_range_map_len;
            continue;
        }
        else if (current_token->type == TOKEN_BEGINCODESPACERANGE)
        {
            int code_range_map_len = strtol(last_token->token, NULL, 10);
            if (cmap->code_range_map == NULL)
            {
                cmap->code_range_map = (pdf_code_range_map_t*)malloc(sizeof(pdf_code_range_map_t) * code_range_map_len);
                if (cmap->code_range_map == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
            }
            else
            {
                pdf_code_range_map_t* t = (pdf_code_range_map_t*)realloc(cmap->code_range_map, sizeof(pdf_code_range_map_t) * (code_range_map_len + cmap->code_range_map_len));
                if (t == NULL)
                {
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                cmap->code_range_map = t;
            }
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
            pdf_parser_token_free(parser, current_token);
            current_token = NULL;
            for (int i = 0; i < code_range_map_len; i++)
            {
                pdf_code_range_map_t* code_range_map = cmap->code_range_map + i + cmap->code_range_map_len;
                pdf_parser_token_t* tk1 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk2 = pdf_parser_next_token(parser);
                code_range_map->srcStart = _hex_str_to_32bit(tk1->token + 1, tk1->token_len - 1);
                code_range_map->srcEnd = _hex_str_to_32bit(tk2->token + 1, tk2->token_len - 1);
                code_range_map->byte_len = (tk1->token_len - 1) / 2;
                pdf_parser_token_free(parser, tk1);
                pdf_parser_token_free(parser, tk2);
            }
            cmap->code_range_map_len += code_range_map_len;
            continue;
        }
        if (last_token != NULL)
        {
            pdf_parser_token_free(parser, last_token);
            last_token = NULL;
        }
        last_token = current_token;
    }
    return cmap;
}
bool _set_common_value(pdf_parser_t* parser, pdf_parser_token_t* tk, struct pdf_value* p)
{
    if (tk->type == TOKEN_NULL)
    {
        p->type = NUL;
    }
    else if (tk->type == TOKEN_ARRAY_BEG)
    {
        pdf_array_t* arr = pdf_parser_build_array(parser);
        if (arr == NULL) return false;
        p->type = ARRAY;
        p->val.array = arr;
    }
    else if (tk->type == TOKEN_DICT_BEG)
    {
        pdf_dict_t* dict = pdf_parser_build_dict(parser);
        if (dict == NULL) return false;
        p->type = DICT;
        p->val.dict = dict;
    }
    else if (tk->type == TOKEN_BOOLEAN_TRUE)
    {
        p->type = BOOLEAN;
        p->val.boolean = true;
    }
    else if (tk->type == TOKEN_BOOLEAN_FALSE)
    {
        p->type = BOOLEAN;
        p->val.boolean = false;
    }
    else if (tk->type == TOKEN_NAME)
    {
        if (tk->token == NULL) return false;
        p->type = NAME;
        p->value_len = tk->token_len;
        p->val.name = (char*)malloc(p->value_len + 1);
        memcpy(p->val.name, tk->token, p->value_len);
        p->val.name[p->value_len] = '\0';
    }
    else if (tk->type == TOKEN_INDIRECT)
    {
        if (tk->token == NULL) return false;
        p->type = INDIRECT;
        char ref[256] = { 0 };
        memcpy(ref, tk->token, tk->token_len);
        char* token = strtok(ref, " ");
        p->val.indirect = atoi(token);
    }
    else if (tk->type == TOKEN_NUMBER)
    {
        if (tk->token == NULL) return false;
        p->type = NUMBER;
        p->val.number = strtod(tk->token, NULL);
    }
    else if (tk->type == TOKEN_STRING || tk->type == TOKEN_HEX_STRING)
    {
        if (tk->token == NULL) return false;
        p->type = STRING;
        p->value_len = tk->token_len;
        p->val.string = (char*)malloc(p->value_len + 1);
        memcpy(p->val.string, tk->token, p->value_len);
        p->val.string[p->value_len] = '\0';
    }
    return true;
}
pdf_obj_t* pdf_parser_build_obj(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;

    pdf_parser_token_t* tk;
    pdf_obj_t* obj = pdf_obj_init();
    obj->value = (pdf_obj_value_t*)malloc(sizeof(pdf_obj_value_t));

    while ((tk = pdf_parser_next_token(parser)) != NULL)
    {
        if (tk->type == TOKEN_OBJ_END)
        {
            pdf_parser_token_free(parser, tk);
            break;
        }
        else if (tk->type == TOKEN_STREAM_BEG)
        {
            input_seek(parser->input, -(parser->end_pos - parser->current_pos + 1), SEEK_CUR);
            char c;
            while (true)
            {
                input_read(parser->input, &c, 1);
                if (c != '\r' && c != '\n')
                {
                    input_seek(parser->input, -1, SEEK_CUR);
                    break;
                }
            }
            // store current offset
            int offset = input_tell(parser->input);
            int len = pdf_dict_get_number(obj->value->val.dict, "/Length");
            if (len == -1)
            {
                int ref = pdf_dict_get_ref(obj->value->val.dict, "/Length");
                if (ref != -1)
                {
                    pdf_obj_t* l_obj = pdf_file_get_obj(parser->pdf, ref);
                    if (l_obj != NULL && l_obj->value->type == NUMBER)
                    {
                        len = l_obj->value->val.number;
                        input_seek(parser->input, offset, SEEK_SET);
                    }
                    else
                    {
                        pdf_parser_token_free(parser, tk);
                        pdf_obj_free(obj);
                        return NULL;
                    }
                } 
            }
            
            obj->stream = pdf_stream_init(parser->pdf, obj, len, offset);
            input_seek(parser->input, len, SEEK_CUR);
            free(parser->remain.rem);
            parser->remain.rem = 0;
            parser->remain.len = 0;
            parser->splite_pos = NULL;
            parser->current_pos = NULL;
            _pdf_parser_read_input(parser);
            pdf_parser_token_free(parser, tk);

            tk = pdf_parser_next_token(parser);
            if (tk == NULL || tk->type != TOKEN_STREAM_END)
            {
                pdf_obj_free(obj);
                return NULL;
            }
        }
        else
        {
            if (!_set_common_value(parser, tk, obj->value))
            {
                pdf_parser_token_free(parser, tk);
                pdf_obj_free(obj);
                return NULL;
            }
        }

        pdf_parser_token_free(parser, tk);
    }

    return obj;
}
pdf_dict_t* pdf_parser_build_dict(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;

    pdf_parser_token_t* tk, * tk1;
    pdf_dict_t* dict = pdf_dict_init();
    while ((tk = pdf_parser_next_token(parser)) != NULL)
    {
        if (tk->type == TOKEN_DICT_END)
        {
            pdf_parser_token_free(parser, tk);
            break;
        }
        else if (tk->token == NULL)
        {
            pdf_parser_token_free(parser, tk);
            pdf_dict_free(dict);
            return NULL;
        }
        tk1 = pdf_parser_next_token(parser);
        if (tk1 == NULL)
        {
            pdf_parser_token_free(parser, tk);
            pdf_dict_free(dict);
            return NULL;
        }
        pdf_dict_pair_t* p = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
        p->name_len = tk->token_len;
        p->name = (char*)malloc(p->name_len + 1);
        memcpy(p->name, tk->token, p->name_len);
        p->name[p->name_len] = '\0';
        p->value = (pdf_dict_pair_value_t*)malloc(sizeof(pdf_dict_pair_value_t));
        if (!_set_common_value(parser, tk1, p->value))
        {
            pdf_parser_token_free(parser, tk);
            pdf_parser_token_free(parser, tk1);
            free(p->name);
            pdf_value_free(p->value);
            free(p);
            pdf_dict_free(dict);
            return NULL;
        }

        pdf_parser_token_free(parser, tk);
        pdf_parser_token_free(parser, tk1);
        cvector_push_back(dict->pairs, p);
    }

    return dict;
}
pdf_array_t* pdf_parser_build_array(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;

    pdf_array_element_value_t** head = NULL;
    int ele_nums = 0;
    int index = 0;
    pdf_parser_token_t* tk;

    while ((tk = pdf_parser_next_token(parser)) != NULL)
    {
        if (tk->type == TOKEN_ARRAY_END)
        {
            pdf_parser_token_free(parser, tk);
            break;
        }
        else
        {
            ele_nums++;
            pdf_array_element_value_t* v = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            if (head == NULL)
            {
                head = (pdf_array_element_value_t**)malloc(sizeof(pdf_array_element_value_t*));
                if (head == NULL)
                {
                    return NULL;
                }
                *head = v;
            }
            else
            {
                pdf_array_element_value_t** pv = (pdf_array_element_value_t**)realloc(head, ele_nums * sizeof(pdf_array_element_value_t*));
                if (pv == NULL)
                {
                    free(head);
                    return NULL;
                }
                head = pv;
                head[index] = v;
            }

            if (!_set_common_value(parser, tk, v))
            {
                pdf_parser_token_free(parser, tk);
                for (int i = 0; i < ele_nums; i++)
                {
                    free(head[i]);
                }
                free(head);
                return NULL;
            }
            index++;
        }


        pdf_parser_token_free(parser, tk);
    }
    pdf_array_t* array = pdf_array_init();
    array->values = head;
    array->num_elements = ele_nums;
    return array;
}