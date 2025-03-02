#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define STRINGFY(x) #x
#define TO_STR(x) STRINGFY(x)
#define pdf_parser_error(fmt, ...) printf(TO_STR(__LINE__) ":"fmt"\n", ##__VA_ARGS__)


const char space_tag[] = {
    '\0', '\t', '\n', '\r', '\f', ' '
};
bool _is_space(char c)
{
    int len = ARRAY_COUNT(space_tag);
    for (int i = 0; i < len; i++)
    {
        if (c == space_tag[i])
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
        && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}
const char delimiter_tag[] = {
    '(', ')', '<', '>', '[', ']', '{', '}', '/', '%'
};
bool _is_delimiter(char c)
{
    int len = ARRAY_COUNT(delimiter_tag);
    for (int i = 0; i < len; i++)
    {
        if (c == delimiter_tag[i])
        {
            return true;
        }
    }

    return false;
}

const static char* operators[] = {
    "\"", "'",
    "B", "B*", "BDC", "BMC", "BI", "BT",
    "CS",
    "DP", "Do",
    "EI", "EMC", "ET",
    "F",
    "G",
    "ID",
    "J",
    "K",
    "M", "MP",
    "Q",
    "RG",
    "S", "SC", "SCN",
    "T*", "TD", "TJ", "TL", "Tc", "Td", "Tf", "Tj", "Tm", "Tr", "Ts", "Tw", "Tz",
    "W", "W*",
    "b", "b*",
    "c", "cm", "cs",
    "d", "d0", "d1",
    "f", "f*",
    "g", "gs",
    "h",
    "i",
    "j",
    "k",
    "l",
    "m",
    "n",
    "q",
    "re", "rg", "ri",
    "s", "sc", "scn", "sh",
    "v",
    "w",
    "y"
};

void pdf_parser_token_free(pdf_parser_token_t* token)
{
    if (token == NULL) return;

    if (token->token)
    {
        free(token->token);
        token->token = NULL;
    }

    free(token);
    token = NULL;
}

pdf_parser_token_t* _parse_number(const unsigned char* start, const unsigned char* end)
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
                    pdf_parser_error("error number: %c", c);
                    return NULL;
                }
            }
            else
            {
                p--; len--;
                break;
            }
        }
        pdf_parser_token_t* tk = pdf_parser_token_init(start, TOKEN_NUMBER, len);

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
                pdf_parser_error("unknown token: %c(%#x)", c, c);
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
        pdf_parser_error("unknown token: %c(%#x)", c, c);
        return NULL;
    }
}

pdf_parser_token_t* pdf_parser_token_init(const unsigned char* start, pdf_parser_token_type_t type, int len)
{
    pdf_parser_token_t* tk = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
    tk->type = type;
    tk->token = (char*)malloc(len + 1);
    memcpy(tk->token, start, len);
    tk->token[len] = '\0';
    tk->token_len = len;
    tk->steps = len;
    tk->next = NULL;

    return tk;
}
const char* pdf_parser_token_get_token(pdf_parser_token_t* token)
{
    if (token == NULL) return NULL;

    return token->token;
}
pdf_parser_t* pdf_parser_init(pdf_file_t* pdf, pdf_parser_reader_type_t type, void* source)
{
    if (pdf == NULL) return NULL;
    pdf_parser_t* parser = (pdf_parser_t*)calloc(1, sizeof(pdf_parser_t));
    parser->pdf = pdf;
    parser->buffer = (unsigned char*)malloc(4096);
    parser->buffer_size = 4096;
    parser->splite_pos = NULL;
    parser->current_pos = NULL;
    parser->end_pos = NULL;
    parser->remain.rem = NULL;
    parser->remain.len = 0;

    parser->reader.type = type;
    parser->reader.source = source;
    switch (type)
    {
        case BUFFER_READER:
            parser->reader.read = _pdf_parser_read_buffer;
            break;
        case FILE_READER:
            parser->reader.read = _pdf_parser_read_file;
            break;
        case STREAM_READER:
            parser->reader.read = _pdf_parser_read_stream;
            break;
        default:
            printf("unknown reader type\n");
            return NULL;
            break;
    }
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
        fseek(parser->pdf->pFile, -remain, SEEK_CUR);
    }
    if (parser->remain.len > 0)
    {
        free(parser->remain.rem);
    }
    free(parser);
    parser = NULL;
}

void _decode_string(char* str, int len, int* out_len)
{
    char* p = str + 1; // skip '('
    char* end = str + len - 1; // skip ')'
    char* out = (char*)malloc(len);
    int ol = 0;
    out[ol++] = *p;
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

void _decode_hex_string(char* str, int len, int* out_len)
{
    char* p = str; // keep '<'
    char* end = str + len - 1; // skip '>'
    char* out = (char*)malloc(len);
    int ol = 0;
    out[ol++] = *p;
    char tmp[2];
    while (p < end)
    {
        tmp[0] = *p;
        if (p + 1 >= end)
        {
            tmp[1] = '0';
        }
        else
        {
            p++;
            tmp[1] = *p;
        }
        char n = (char)strtol(tmp, NULL, 16);
        if (!_is_space(n))
            out[ol++] = n;
        p++;
    }
    memcpy(str, out, ol);
    str[ol] = '\0';
    *out_len = ol;
    free(out);
}

pdf_parser_token_t* _pdf_parser_next_one_token(const unsigned char* start, const unsigned char* end)
{
    if (start == NULL)
        return NULL;
    pdf_parser_token_t* tk = NULL;
    unsigned char c = *start;
    do {
        int len = 1;
        const unsigned char* p = start;
        while (p < end)
        {
            p++; len++;
            c = *p;
            if (_is_space(c) || _is_delimiter(c))
            {
                p--; len--;
                break;
            }
        }

        char* to_compare = (char*)malloc(len + 1);
        memcpy(to_compare, start, len);
        to_compare[len] = '\0';
        int operator_count = ARRAY_COUNT(operators);
        bool is_operator = false;

        int left = 0;
        int right = operator_count - 1;

        while (left <= right)
        {
            int mid = left + (right - left) / 2;
            int cmp = strcmp(operators[mid], to_compare);
            if (cmp < 0)
            {
                left = mid + 1;
            }
            else if (cmp > 0)
            {
                right = mid - 1;
            }
            else
            {
                is_operator = true;
                break;
            }
        }
        free(to_compare);
        if (is_operator)
        {
            tk = pdf_parser_token_init(start, TOKEN_OPERATOR, len);
            start += len;
            return tk;
        }
    } while (0);
    c = *start;
    if (c == -1)
    {
        return NULL;
    }
    else if (c == '/') // parse name
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

        tk = pdf_parser_token_init(start, TOKEN_NAME, len);
        start += len;

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
            else if (c == -1)
            {
                pdf_parser_error("unexcepted end of string");
                return NULL;
            }
        }

        tk = pdf_parser_token_init(start, TOKEN_STRING, len);
        tk->token[tk->token_len - 1] = '\0';
        tk->token_len--;
        start += len;
        // decode string
        //_decode_string(tk->token, tk->token_len, &tk->token_len);
    }
    else if (c == '[')
    {
        int len = 1;
        tk = pdf_parser_token_init(start, TOKEN_ARRAY_BEG, len);
        start += len;
    }
    else if (c == ']')
    {
        int len = 1;
        tk = pdf_parser_token_init(start, TOKEN_ARRAY_END, len);
        start += len;

    }
    else if (c == '<')
    {
        if (memcmp(start, "<<", 2) == 0) // dict
        {
            int len = 2;
            tk = pdf_parser_token_init(start, TOKEN_DICT_BEG, len);
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
                if (_is_hex(c))
                {
                    continue;
                }
                else if (c == '>')
                {
                    break;
                }
            }

            tk = pdf_parser_token_init(start, TOKEN_HEX_STRING, len);
            tk->token[tk->token_len - 1] = '\0';
            tk->token_len--;
            start += len;
            // decode hex string
            //_decode_hex_string(tk->token, tk->token_len, &tk->token_len);
        }
    }
    else if (c == '>')
    {
        if (memcmp(start, ">>", 2) == 0) // dict
        {
            int len = 2;
            tk = pdf_parser_token_init(start, TOKEN_DICT_END, len);
            start += len;

        }
        else
        {
            pdf_parser_error("unexcepted tag >");
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
        tk = pdf_parser_token_init(start, TOKEN_COMMENT, len);
        start += len;
    }
    else if (c == 'b')
    {
        if (memcmp(start, "begincodespacerange", 19) == 0)
        {
            int len = 19;
            tk = pdf_parser_token_init(start, TOKEN_BEGINCODESPACERANGE, len);
            start += len;
        }
        else if (memcmp(start, "begincidrange", 13) == 0)
        {
            int len = 13;
            tk = pdf_parser_token_init(start, TOKEN_BEGINCIDRANGE, len);
            start += len;
        }
        else if (memcmp(start, "beginbfrange", 12) == 0)
        {
            int len = 12;
            tk = pdf_parser_token_init(start, TOKEN_BEGINBFRANGE, len);
            start += len;
        }
        else if (memcmp(start, "begincidchar", 12) == 0)
        {
            int len = 12;
            tk = pdf_parser_token_init(start, TOKEN_BEGINCIDCHAR, len);
            start += len;
        }
        else if (memcmp(start, "beginbfchar", 11) == 0)
        {
            int len = 11;
            tk = pdf_parser_token_init(start, TOKEN_BEGINBFCHAR, len);
            start += len;
        }
        else if (memcmp(start, "begincmap", 9) == 0)
        {
            int len = 9;
            tk = pdf_parser_token_init(start, TOKEN_BEGINCMAP, len);
            start += len;
        }
        else if (memcmp(start, "begin", 5) == 0)
        {
            int len = 5;
            tk = pdf_parser_token_init(start, TOKEN_BEGIN, len);
            start += len;
        }

    }
    else if (c == 'd')
    {
        if (memcmp(start, "dict", 4) == 0)
        {
            int len = 4;
            tk = pdf_parser_token_init(start, TOKEN_DICT, len);
            start += len;
        }
        else if (memcmp(start, "def", 3) == 0)
        {
            int len = 3;
            tk = pdf_parser_token_init(start, TOKEN_DEF, len);
            start += len;
        }
        else if (memcmp(start, "dup", 3) == 0)
        {
            int len = 3;
            tk = pdf_parser_token_init(start, TOKEN_DUP, len);
            start += len;
        }
    }
    else if (c == 'e')
    {
        if (memcmp(start, "endcodespacerange", 17) == 0)
        {
            int len = 17;
            tk = pdf_parser_token_init(start, TOKEN_ENDCODESPACERANGE, len);
            start += len;
        }
        else if (memcmp(start, "endcidrange", 11) == 0)
        {
            int len = 11;
            tk = pdf_parser_token_init(start, TOKEN_ENDCIDRANGE, len);
            start += len;
        }
        else if (memcmp(start, "endbfrange", 10) == 0)
        {
            int len = 10;
            tk = pdf_parser_token_init(start, TOKEN_ENDBFRANGE, len);
            start += len;
        }
        else if (memcmp(start, "endcidchar", 10) == 0)
        {
            int len = 10;
            tk = pdf_parser_token_init(start, TOKEN_ENDCIDCHAR, len);
            start += len;
        }
        else if (memcmp(start, "endstream", 9) == 0)
        {
            int len = 9;
            tk = pdf_parser_token_init(start, TOKEN_STREAM_END, len);
            start += len;
        }
        else if (memcmp(start, "endbfchar", 9) == 0)
        {
            int len = 9;
            tk = pdf_parser_token_init(start, TOKEN_ENDBFCHAR, len);
            start += len;
        }
        else if (memcmp(start, "endcmap", 7) == 0)
        {
            int len = 7;
            tk = pdf_parser_token_init(start, TOKEN_ENDCMAP, len);
            start += len;
        }
        else if (memcmp(start, "endobj", 6) == 0)
        {
            int len = 6;
            tk = pdf_parser_token_init(start, TOKEN_OBJ_END, len);
            start += len;
        }
        else if (memcmp(start, "end", 3) == 0)
        {
            int len = 3;
            tk = pdf_parser_token_init(start, TOKEN_OBJ_END, len);
            start += len;
        }
    }
    else if (c == 'f')
    {
        if (memcmp(start, "false", 5) == 0)
        {
            int len = 5;
            tk = pdf_parser_token_init(start, TOKEN_BOOLEAN_FALSE, len);
            start += len;
        }
        else if (memcmp(start, "findresource", 12) == 0)
        {
            int len = 12;
            tk = pdf_parser_token_init(start, TOKEN_FINDRESOURCE, len);
            start += len;
        }
    }
    else if (c == 'o')
    {
        if (memcmp(start, "obj", 3) == 0)
        {
            int len = 3;
            tk = pdf_parser_token_init(start, TOKEN_OBJ_BEG, len);
            start += len;

        }
    }
    else if (c == 's')
    {
        if (memcmp(start, "stream", 6) == 0)
        {
            int len = 6;

            tk = pdf_parser_token_init(start, TOKEN_STREAM_BEG, len);
            start += len;

        }
    }
    else if (c == 'n')
    {
        if (memcmp(start, "null", 4) == 0)
        {
            int len = 4;
            tk = pdf_parser_token_init(start, TOKEN_NULL, len);
            start += len;
        }
    }
    else if (c == 't')
    {
        if (memcmp(start, "true", 4) == 0)
        {
            int len = 4;
            tk = pdf_parser_token_init(start, TOKEN_BOOLEAN_TRUE, len);
            start += len;
        }
    }
    else if (c == 'x')
    {
        if (memcmp(start, "xref", 4) == 0)
        {
            int len = 4;
            tk = pdf_parser_token_init(start, TOKEN_XREF, len);
            start += len;
        }
    }
    else if (c == 'R')
    {
        int len = 1;
        tk = pdf_parser_token_init(start, TOKEN_INDIRECT, len);
        start += len;
    }
    else if (c == '-' || c == '+' || c == '.' || _is_digit(c))
    {

        tk = _parse_number(start, end);
        start += tk->steps;
    }
    else if (_is_space(c))
    {
        if (memcmp(start, "\r\n", 2) == 0)
        {
            tk = pdf_parser_token_init(start, TOKEN_NEWLINE, 2);
            start += tk->steps;
        }
        else
        {
            tk = pdf_parser_token_init(start, TOKEN_SPACE, 1);
            start += tk->steps;
        }
    }
    else
    {
        pdf_parser_error("unknow token: %c(%#x)", *start, *start);
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
void _pdf_parser_read_file(pdf_parser_t* parser, void* source)
{
    FILE* f = (FILE*)source;
    if (parser == NULL)
    {
        return;
    }
    int off = _pdf_parser_copy_rem(parser);
    int ret = 0;
    ret = fread(parser->buffer + off, 1, parser->buffer_size - off, parser->pdf->pFile);
    if (ret < 0)
    {
        return;
    }
    int end_i = off + ret - 1;
    _pdf_parser_split(parser, end_i);
}
void _pdf_parser_read_buffer(pdf_parser_t* parser, void* source)
{
    pdf_buffer_t* input = (pdf_buffer_t*)source;
    if (parser == NULL)
    {
        return;
    }
    int off = _pdf_parser_copy_rem(parser);
    int ret = 0;
    if (parser != NULL && input != NULL)
    {
        if (input->processed < input->buffer_size)
        {
            ret = MIN(parser->buffer_size - off, input->buffer_size - input->processed);
            memcpy(parser->buffer + off, input->buffer + input->processed, ret);
            input->processed += ret;
        }
    }

    int end_i = off + ret - 1;
    _pdf_parser_split(parser, end_i);
}
void _pdf_parser_read_stream(pdf_parser_t* parser, void* source)
{
    pdf_stream_t* stream = (pdf_stream_t*)source;
    if (parser == NULL)
    {
        return;
    }
    int off = _pdf_parser_copy_rem(parser);
    int ret = 0;
    if (parser != NULL && stream != NULL)
    {
        if ((stream->decomp.cur_pos >= stream->decomp.len && stream->readin_len < stream->stream_len)
            || stream->processed < stream->stream_len)
        {
            ret = pdf_stream_get_data(stream, parser->buffer + off, parser->buffer_size - off);
        }
    }

    int end_i = off + ret - 1;
    _pdf_parser_split(parser, end_i);
}

pdf_parser_token_t* _pdf_next_token(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;

    pdf_parser_token_t* head = (pdf_parser_token_t*)calloc(1, sizeof(pdf_parser_token_t));
    head->next = NULL;
    pdf_parser_token_t* tmp = head;
    unsigned char** start = &(parser->current_pos);
    unsigned char* end = parser->splite_pos;
    int steps = 0;

    while (*start < end)
    {
        pdf_parser_token_t* tk = _pdf_parser_next_one_token(*start, end);
        if (tk == NULL)
        {
            break;
        }
        // tokens that need return 
        else if (
            tk->type == TOKEN_NULL
            || tk->type == TOKEN_NAME
            || tk->type == TOKEN_STRING
            || tk->type == TOKEN_BOOLEAN_TRUE
            || tk->type == TOKEN_BOOLEAN_FALSE
            || tk->type == TOKEN_HEX_STRING
            || tk->type == TOKEN_ARRAY_BEG
            || tk->type == TOKEN_ARRAY_END
            || tk->type == TOKEN_DICT_BEG
            || tk->type == TOKEN_DICT_END
            || tk->type == TOKEN_OBJ_BEG
            || tk->type == TOKEN_OBJ_END
            || tk->type == TOKEN_STREAM_BEG
            || tk->type == TOKEN_STREAM_END
            || tk->type == TOKEN_INDIRECT
            || tk->type == TOKEN_OPERATOR
            || tk->type == TOKEN_XREF
            || tk->type == TOKEN_FINDRESOURCE
            || tk->type == TOKEN_BEGIN
            || tk->type == TOKEN_DICT
            || tk->type == TOKEN_BEGINCMAP
            || tk->type == TOKEN_DEF
            || tk->type == TOKEN_DUP
            || tk->type == TOKEN_END
            || tk->type == TOKEN_BEGINCODESPACERANGE
            || tk->type == TOKEN_ENDCODESPACERANGE
            || tk->type == TOKEN_BEGINBFCHAR
            || tk->type == TOKEN_ENDBFCHAR
            || tk->type == TOKEN_ENDCMAP
            || tk->type == TOKEN_BEGINCIDCHAR
            || tk->type == TOKEN_ENDCIDCHAR
            || tk->type == TOKEN_BEGINBFRANGE
            || tk->type == TOKEN_ENDBFRANGE
            || tk->type == TOKEN_BEGINCIDRANGE
            || tk->type == TOKEN_ENDCIDRANGE
            )
        {
            tmp->next = tk;
            tmp = tmp->next;

            head->type = tk->type;
            *start += tk->steps;
            steps += tk->steps;
            break;
        }
        else if (tk->type == TOKEN_NUMBER)
        {
            int count = 0;
            if ((*start + tk->steps) >= end)
            {
                parser->reader.read(parser, parser->reader.source);
            }

            pdf_parser_token_t* next = NULL;
            bool need_ret = true;
            const unsigned char* p;
        CHECK_IS_INDIRECT_OR_OBJ:
            p = *start + tk->steps;
            // check next two tokens
            while (p < end && count < 2 && (next = _pdf_parser_next_one_token(p, end)) != NULL)
            {
                if (next->type == TOKEN_INDIRECT || next->type == TOKEN_OBJ_BEG)
                {
                    need_ret = false;
                    pdf_parser_token_free(next);
                    break;
                }
                else if (next->type == TOKEN_SPACE)
                {
                    // ignore
                }
                else
                {
                    count++;
                }
                p += next->steps;
                pdf_parser_token_free(next);
            }
            if (p >= end && count < 2)
            {
                parser->reader.read(parser, parser->reader.source);
                if (parser->current_pos <= parser->splite_pos)
                {
                    start = &(parser->current_pos);
                    end = parser->splite_pos;
                    goto CHECK_IS_INDIRECT_OR_OBJ;
                }
            }
            *start += tk->steps;
            steps += tk->steps;
            tmp->next = tk;
            tmp = tmp->next;

            head->type = tk->type;
            if (need_ret)
            {
                break;
            }
        }
        else if (tk->type == TOKEN_SPACE || tk->type == TOKEN_COMMENT)
        {
            // ignored
            *start += tk->steps;
            steps += tk->steps;
            pdf_parser_token_free(tk);
            continue;
        }
        else if (tk->type == TOKEN_NEWLINE)
        {
            *start += tk->steps;
            steps += tk->steps;
            pdf_parser_token_free(tk);
            if (head->next == NULL)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }

    tmp = head->next;
    if (tmp == NULL)
    {
        pdf_parser_token_free(head);
        return NULL;
    }
    if (head->type != TOKEN_OBJ_BEG && head->type != TOKEN_INDIRECT)
    {
        pdf_parser_token_free(head);
        return tmp;
    }
    else
    {

        int len = 0;
        int count = 0; // space needed
        for (; tmp; tmp = tmp->next)
        {
            len += tmp->token_len;
            count++;
        }

        tmp = head->next;
        pdf_parser_token_t* token = (pdf_parser_token_t*)malloc(sizeof(pdf_parser_token_t));
        token->type = head->type;
        token->token = (char*)malloc(len + count + 1);
        token->steps = steps;
        pdf_parser_token_free(head);
        int off = 0;
        pdf_parser_token_t* p;
        for (; tmp; )
        {
            p = tmp->next;
            memcpy(token->token + off, tmp->token, tmp->token_len);
            off += tmp->token_len;
            if (p != NULL)
            {
                token->token[off] = ' ';
                off += 1;
            }
            else
            {
                token->token[off] = '\0';
            }
            pdf_parser_token_free(tmp);
            tmp = p;
        }
        token->token_len = off;
        return token;
    }
}
pdf_parser_token_t* pdf_stream_get_next_token(pdf_stream_t* stream)
{
    return pdf_parser_next_token(stream->parser);
}

pdf_parser_token_t* pdf_parser_next_token(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;
    if (parser->pdf == NULL)
        return NULL;
    if (parser->pdf->pFile == NULL)
        return NULL;

    unsigned char* save_cur = parser->current_pos;
    if (parser->current_pos >= parser->splite_pos)
    {
        parser->reader.read(parser, parser->reader.source);
    }
    pdf_parser_token_t* tk = _pdf_next_token(parser);
    if (tk == NULL)
    {
        if (parser->reader.type == STREAM_READER)
        {
            pdf_stream_t* s = (pdf_stream_t*)parser->reader.source;
            if (s->processed < s->stream_len)
            {
                parser->current_pos = save_cur;
                parser->reader.read(parser, parser->reader.source);
                tk = _pdf_next_token(parser);
            }
        }
    }
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
            pdf_parser_token_free(last_token);
            last_token = NULL;
            pdf_parser_token_free(current_token);
            current_token = NULL;
            break;
        }
        else if (current_token->type == TOKEN_BEGINBFCHAR || current_token->type == TOKEN_BEGINCIDCHAR)
        {
            // single char mapping
            bool isCid = (current_token->type == TOKEN_BEGINCIDCHAR);
            int unicode_map_len = strtol(last_token->token, NULL, 10);
            pdf_unicode_map_t* unicode_map = (pdf_unicode_map_t*)calloc(unicode_map_len, sizeof(pdf_unicode_map_t));
            pdf_parser_token_free(last_token);
            last_token = NULL;
            pdf_parser_token_free(current_token);
            current_token = NULL;

            for (int i = 0; i < unicode_map_len; i++)
            {
                last_token = pdf_parser_next_token(parser);
                current_token = pdf_parser_next_token(parser);
                unicode_map[i].cid = _hex_str_to_16bit(last_token->token + 1);
                if (isCid)
                {
                    unicode_map[i].unicode = (uint16_t)strtol(current_token->token + 1, NULL, 10);
                }
                else
                {
                    unicode_map[i].unicode = _hex_str_to_16bit(current_token->token + 1);
                }

                pdf_parser_token_free(last_token);
                last_token = NULL;
                pdf_parser_token_free(current_token);
                current_token = NULL;
            }
            if (cmap->unicode_map_len == 0)
            {
                cmap->unicode_map = unicode_map;
                cmap->unicode_map_len = unicode_map_len;
            }
            else
            {
                pdf_unicode_map_t* tmp = (pdf_unicode_map_t*)realloc(cmap->unicode_map,
                    sizeof(pdf_unicode_map_t) * (cmap->unicode_map_len + unicode_map_len));
                if (tmp == NULL)
                {
                    free(unicode_map);
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                memcpy(tmp + cmap->unicode_map_len, unicode_map, sizeof(pdf_unicode_map_t) * unicode_map_len);
                free(unicode_map);
                cmap->unicode_map = tmp;
                cmap->unicode_map_len += unicode_map_len;
                
            }
            continue;
        }
        else if (current_token->type == TOKEN_BEGINBFRANGE || current_token->type == TOKEN_BEGINCIDRANGE)
        {
            bool isCid = (current_token->type == TOKEN_BEGINCIDRANGE);
            int char_range_map_len = strtol(last_token->token, NULL, 10);
            pdf_char_range_map_t* char_range_map = (pdf_char_range_map_t*)calloc(char_range_map_len, sizeof(pdf_char_range_map_t));
            pdf_parser_token_free(last_token);
            last_token = NULL;
            pdf_parser_token_free(current_token);
            current_token = NULL;
            for (int i = 0; i < char_range_map_len; i++)
            {
                pdf_parser_token_t* tk1 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk2 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk3 = pdf_parser_next_token(parser);
                char_range_map[i].srcStart = _hex_str_to_16bit(tk1->token + 1);
                char_range_map[i].srcEnd = _hex_str_to_16bit(tk2->token + 1);
                if (isCid)
                {
                    char_range_map[i].dstStart = (uint16_t)strtol(tk3->token + 1, NULL, 10);
                }
                else
                {
                    char_range_map[i].dstStart = _hex_str_to_16bit(tk3->token + 1);
                }
                pdf_parser_token_free(tk1);
                pdf_parser_token_free(tk2);
                pdf_parser_token_free(tk3);
            }
            if (cmap->char_range_map_len == 0)
            {
                cmap->char_range_map = char_range_map;
                cmap->char_range_map_len = char_range_map_len;
            }
            else
            {
                pdf_char_range_map_t* t = (pdf_char_range_map_t*)realloc(cmap->char_range_map, 
                    sizeof(pdf_char_range_map_t) * (cmap->char_range_map_len + char_range_map_len));
                if (t == NULL)
                {
                    free(char_range_map);
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                    
                memcpy(t + cmap->char_range_map_len, char_range_map, sizeof(pdf_char_range_map_t) * char_range_map_len);
                free(char_range_map);
                cmap->char_range_map = t;
                cmap->char_range_map_len += char_range_map_len;
            }
            continue;
        }
        else if (current_token->type == TOKEN_BEGINCODESPACERANGE)
        {
            int code_range_map_len = strtol(last_token->token, NULL, 10);
            pdf_code_range_map_t* code_range_map = (pdf_code_range_map_t*)calloc(code_range_map_len, sizeof(pdf_code_range_map_t));
            pdf_parser_token_free(last_token);
            last_token = NULL;
            pdf_parser_token_free(current_token);
            current_token = NULL;
            for (int i = 0; i < code_range_map_len; i++)
            {
                pdf_parser_token_t* tk1 = pdf_parser_next_token(parser);
                pdf_parser_token_t* tk2 = pdf_parser_next_token(parser);
                code_range_map[i].srcStart = _hex_str_to_16bit(tk1->token + 1);
                code_range_map[i].srcEnd = _hex_str_to_16bit(tk2->token + 1);
                pdf_parser_token_free(tk1);
                pdf_parser_token_free(tk2);
            }
            if (cmap->code_range_map_len == 0)
            {
                cmap->code_range_map = code_range_map;
                cmap->code_range_map_len = code_range_map_len;
            }
            else
            {
                pdf_code_range_map_t* t = (pdf_code_range_map_t*)realloc(cmap->code_range_map,
                sizeof(pdf_code_range_map_t) * (cmap->code_range_map_len * code_range_map_len));
                if (t == NULL)
                {
                    free(code_range_map);
                    pdf_cmap_free(cmap);
                    return NULL;
                }
                    
                memcpy(t + cmap->code_range_map_len, code_range_map, sizeof(pdf_code_range_map_t) * code_range_map_len);
                free(code_range_map);
                cmap->code_range_map = t;
                cmap->code_range_map_len += code_range_map_len;
            }
            continue;
        }
        if (last_token != NULL)
        {
            pdf_parser_token_free(last_token);
            last_token = NULL;
        }
        last_token = current_token;
    }
    return cmap;
}
void _set_common_value(pdf_parser_t* parser, pdf_parser_token_t* tk, struct pdf_value* p)
{
    if (tk->type == TOKEN_NULL)
    {
        p->type = NUL;
    }
    else if (tk->type == TOKEN_ARRAY_BEG)
    {
        p->type = ARRAY;
        p->val.array = pdf_parser_build_array(parser);
    }
    else if (tk->type == TOKEN_DICT_BEG)
    {
        p->type = DICT;
        p->val.dict = pdf_parser_build_dict(parser);
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
        p->type = NAME;
        p->value_len = tk->token_len;
        p->val.name = (char*)malloc(p->value_len + 1);
        memcpy(p->val.name, tk->token, p->value_len);
        p->val.name[p->value_len] = '\0';
    }
    else if (tk->type == TOKEN_INDIRECT)
    {
        p->type = INDIRECT;
        char ref[256] = { 0 };
        memcpy(ref, tk->token, tk->token_len);
        char* token = strtok(ref, " ");
        p->val.indirect = atoi(token);
    }
    else if (tk->type == TOKEN_NUMBER)
    {
        p->type = NUMBER;
        p->val.number = strtod(tk->token, NULL);
    }
    else if (tk->type == TOKEN_STRING || tk->type == TOKEN_HEX_STRING)
    {
        p->type = STRING;
        p->value_len = tk->token_len;
        p->val.string = (char*)malloc(p->value_len + 1);
        memcpy(p->val.string, tk->token, p->value_len);
        p->val.string[p->value_len] = '\0';
    }
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
            pdf_parser_token_free(tk);
            break;
        }
        else if (tk->type == TOKEN_STREAM_BEG)
        {
            fseek(parser->pdf->pFile, -(parser->end_pos - parser->current_pos), SEEK_CUR);
            char c;
            while (true)
            {
                c = getc(parser->pdf->pFile);
                if (c != '\r' && c != '\n')
                {
                    fseek(parser->pdf->pFile, -1, SEEK_CUR);
                    break;
                }
            }
            // store current offset
            int offset = ftell(parser->pdf->pFile);
            int len = pdf_dict_get_number(obj->value->val.dict, "/Length");
            if (len == -1)
            {
                int ref = pdf_dict_get_ref(obj->value->val.dict, "/Length");
                pdf_obj_t* l_obj = pdf_file_get_obj(parser->pdf, ref);
                len = l_obj->value->val.number;
                fseek(parser->pdf->pFile, offset, SEEK_SET);
            }
            //int offset = ftell(parser->pdf->pFile);
            obj->stream = pdf_stream_init(parser->pdf, obj, len, offset);
            fseek(parser->pdf->pFile, len, SEEK_CUR);
            free(parser->remain.rem);
            parser->remain.rem = 0;
            parser->remain.len = 0;
            parser->splite_pos = NULL;
            parser->current_pos = NULL;
            parser->reader.read(parser, parser->reader.source);
            pdf_parser_token_free(tk);

            tk = pdf_parser_next_token(parser);
            if (tk == NULL || tk->type != TOKEN_STREAM_END)
            {
                pdf_obj_free(obj);
                return NULL;
            }
        }
        else
        {
            _set_common_value(parser, tk, obj->value);
        }

        pdf_parser_token_free(tk);
    }

    return obj;
}
pdf_dict_t* pdf_parser_build_dict(pdf_parser_t* parser)
{
    if (parser == NULL)
        return NULL;
    pdf_dict_pair_t* p = NULL;
    pdf_dict_pair_t** head = NULL;
    int num_pairs = 0;
    int index = 0;
    pdf_parser_token_t* tk;

    bool is_name = true;
    while ((tk = pdf_parser_next_token(parser)) != NULL)
    {
        if (tk->type == TOKEN_DICT_END)
        {
            pdf_parser_token_free(tk);
            break;
        }
        if (is_name)
        {
            num_pairs++;
            pdf_dict_pair_t* v = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
            if (head == NULL)
            {
                head = (pdf_dict_pair_t**)malloc(sizeof(pdf_dict_pair_t*));
                if (head == NULL)
                {
                    return NULL;
                }
                *head = v;
            }
            else
            {
                pdf_dict_pair_t** pv = (pdf_dict_pair_t**)realloc(head, num_pairs * sizeof(pdf_dict_pair_t*));
                if (pv == NULL)
                {
                    free(head);
                    return NULL;
                }
                head = pv;
                head[index] = v;
            }
            p = v;

            p->name_len = tk->token_len;
            p->name = (char*)malloc(p->name_len + 1);
            memcpy(p->name, tk->token, p->name_len);
            p->name[p->name_len] = '\0';
            p->value = (pdf_dict_pair_value_t*)malloc(sizeof(pdf_dict_pair_value_t));

            is_name = false;
        }
        else
        {
            _set_common_value(parser, tk, p->value);
            is_name = true;
            index++;
        }

        pdf_parser_token_free(tk);
    }
    pdf_dict_t* dict = pdf_dict_init();
    if (dict == NULL)
        return NULL;
    dict->pairs = head;
    dict->num_pairs = num_pairs;
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
            pdf_parser_token_free(tk);
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

            _set_common_value(parser, tk, v);
            index++;
        }


        pdf_parser_token_free(tk);
    }
    pdf_array_t* array = pdf_array_init();
    array->values = head;
    array->num_elements = ele_nums;
    return array;
}