#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

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
    const char *token;
    PdfTokenType type;
    int len;
} fixed_token_t;
typedef struct 
{
    int nums;
    fixed_token_t fixed_token_map[40];
} fixed_token_map_t;
#define MAX_FIXED_TOKEN_LEN 19
const static fixed_token_map_t fixed_token_map[MAX_FIXED_TOKEN_LEN + 1] = 
{
    {0},
    {31, {
        {"\"", TOKEN_OPERATOR, 1}, // 34 
        {"'", TOKEN_OPERATOR, 1}, // 39
        {"B", TOKEN_OPERATOR, 1},// 66
         {"F", TOKEN_OPERATOR, 1}, // 70
         {"G", TOKEN_OPERATOR, 1}, // 71
         {"J", TOKEN_OPERATOR, 1}, // 74
         {"K", TOKEN_OPERATOR, 1}, // 75
         {"M", TOKEN_OPERATOR, 1}, // 77
         {"Q", TOKEN_OPERATOR, 1}, // 81
        {"R", TOKEN_INDIRECT, 1}, // 82
        {"S", TOKEN_OPERATOR, 1}, // 83
        {"W", TOKEN_OPERATOR, 1}, // 87
        {"[", TOKEN_ARRAY_BEG, 1}, // 91
        {"[", TOKEN_ARRAY_END, 1}, // 93
        {"b", TOKEN_OPERATOR, 1}, //98
        {"c", TOKEN_OPERATOR, 1}, // 99
        {"d", TOKEN_OPERATOR, 1}, // 100
        {"f", TOKEN_OPERATOR, 1}, // 102
        {"g", TOKEN_OPERATOR, 1},  //103
        {"h", TOKEN_OPERATOR, 1}, // 104
        {"i", TOKEN_OPERATOR, 1}, // 105
        {"j", TOKEN_OPERATOR, 1}, // 106
        {"k", TOKEN_OPERATOR, 1}, // 107
        {"l", TOKEN_OPERATOR, 1}, // 108
        {"m", TOKEN_OPERATOR, 1}, //109
        {"n", TOKEN_OPERATOR, 1}, // 110
        {"q", TOKEN_OPERATOR, 1}, // 113
        {"s",TOKEN_OPERATOR, 1}, // 115
        {"v", TOKEN_OPERATOR, 1}, // 118
        {"w", TOKEN_OPERATOR, 1}, // 119
        {"y", TOKEN_OPERATOR, 1} // 121
    }},
    {40, {
        {"<<", TOKEN_DICT_BEG, 2}, // 60
        {">>", TOKEN_DICT_END, 2}, // 62
        {"B*", TOKEN_OPERATOR, 2}, 
        {"BI", TOKEN_OPERATOR, 2}, 
        {"BT", TOKEN_OPERATOR, 2}, 
        {"CS", TOKEN_OPERATOR, 2}, 
        {"DP", TOKEN_OPERATOR, 2},
        {"Do", TOKEN_OPERATOR, 2}, 
        {"EI", TOKEN_OPERATOR, 2}, 
        {"ET", TOKEN_OPERATOR, 2}, 
        {"ID", TOKEN_OPERATOR, 2}, 
        {"MP", TOKEN_OPERATOR, 2},
        {"RG", TOKEN_OPERATOR, 2}, 
        {"SC", TOKEN_OPERATOR, 2}, 
        {"T*", TOKEN_OPERATOR, 2}, 
        {"TD", TOKEN_OPERATOR, 2}, 
        {"TJ", TOKEN_OPERATOR, 2},
        {"TL", TOKEN_OPERATOR, 2}, 
        {"Tc", TOKEN_OPERATOR, 2}, 
        {"Td", TOKEN_OPERATOR, 2}, 
        {"Tf", TOKEN_OPERATOR, 2}, 
        {"Tj", TOKEN_OPERATOR, 2},
        {"Tm", TOKEN_OPERATOR, 2}, 
        {"Tr", TOKEN_OPERATOR, 2}, 
        {"Ts", TOKEN_OPERATOR, 2}, 
        {"Tw", TOKEN_OPERATOR, 2}, 
        {"Tz", TOKEN_OPERATOR, 2},
        {"W*", TOKEN_OPERATOR, 2}, 
        {"b*", TOKEN_OPERATOR, 2}, 
        {"cm", TOKEN_OPERATOR, 2}, 
        {"cs", TOKEN_OPERATOR, 2}, 
        {"d0", TOKEN_OPERATOR, 2},
        {"d1", TOKEN_OPERATOR, 2}, 
        {"f*", TOKEN_OPERATOR, 2}, 
        {"gs", TOKEN_OPERATOR, 2}, 
        {"re", TOKEN_OPERATOR, 2}, 
        {"rg", TOKEN_OPERATOR, 2},
        {"ri", TOKEN_OPERATOR, 2}, 
        {"sc", TOKEN_OPERATOR, 2}, 
        {"sh", TOKEN_OPERATOR, 2},
    }},
    {9, {
        {"BDC", TOKEN_OPERATOR, 3}, 
        {"BMC", TOKEN_OPERATOR, 3}, 
        {"EMC", TOKEN_OPERATOR, 3}, 
        {"SCN", TOKEN_OPERATOR, 3},
        {"def", TOKEN_DEF, 3},
        {"dup", TOKEN_DUP, 3},
        {"end", TOKEN_END, 3},
        {"obj", TOKEN_OBJ_BEG, 3},
        {"scn", TOKEN_OPERATOR, 3}
    }},
    {4, {
        {"dict", TOKEN_DICT, 4},
        {"null", TOKEN_NULL, 4},
        {"true", TOKEN_BOOLEAN_TRUE, 4},
        {"xref", TOKEN_XREF, 4},
    }},
    {2,{
        {"begin", TOKEN_BEGIN, 5},
        {"false", TOKEN_BOOLEAN_FALSE, 5}
    }},
    {2, {
        {"endobj", TOKEN_OBJ_END, 6},
        {"stream", TOKEN_STREAM_BEG, 6},
    }},
    {1, {
        {"endcmap", TOKEN_ENDCMAP, 7},
    }},
    {0},
    {3, {
        {"begincmap",TOKEN_BEGINCMAP, 9},
        {"endbfchar",TOKEN_ENDBFCHAR, 9},
        {"endstream",TOKEN_STREAM_END, 9},
    }},
    {2, {
        {"endbfrange",TOKEN_ENDBFRANGE, 10},
        {"endcidchar",TOKEN_ENDCIDCHAR, 10},
    }},
    {2, {
        {"beginbfchar",TOKEN_BEGINBFCHAR, 11},
        {"endcidrange",TOKEN_ENDCIDRANGE, 11},
    }},
    {3, {
        {"beginbfrange",TOKEN_BEGINBFRANGE, 12},
        {"begincidchar",TOKEN_BEGINCIDCHAR, 12},
        {"findresource",TOKEN_FINDRESOURCE, 12},
    }},
    {1, {
        {"begincidrange", TOKEN_BEGINCIDRANGE, 13},
    }},
    {0}, {0}, {0},
    {1, {
        {"endcodespacerange",TOKEN_ENDCODESPACERANGE, 17},
    }}, {0},
    {1, {
        {"begincodespacerange",TOKEN_BEGINCODESPACERANGE, 19},
    }}
};
PdfToken::~PdfToken()
{
    if (token)
    {
        free(token);
    }
}

PdfToken* PdfParser::_buildNumber(const unsigned char* start, const unsigned char* end)
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
                    printf("error number: %c", c);
                    return NULL;
                }
            }
            else
            {
                p--; len--;
                break;
            }
        }
        PdfToken* tk = _buildToken(start, TOKEN_NUMBER, len);

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
                printf("unknown token: %c(%#x)", c, c);
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
        printf("unknown token: %c(%#x)", c, c);
        return NULL;
    }
}

PdfToken* PdfParser::_buildToken(const unsigned char* start, PdfTokenType type, int len)
{
    PdfToken* tk = NULL;
    if (len > MAX_FIXED_TOKEN_LEN)
    {
        tk = new PdfToken;
        tk->token = (char*)malloc(len + 1);
    }
    else if (freedTokens == NULL)
    {
        tk = new PdfToken;
        tk->token = (char*)malloc(MAX_FIXED_TOKEN_LEN + 1);
    }
    else
    {
        tk = freedTokens;
        freedTokens = freedTokens->next;
    }
    tk->type = type;
    memcpy(tk->token, start, len);
    tk->token[len] = '\0';
    tk->token_len = len;
    tk->steps = len;
    tk->next = NULL;
    return tk;
}
void PdfParser::freeToken(PdfToken* token)
{
    if (token == NULL) return;
    if (token->token_len < MAX_FIXED_TOKEN_LEN)
    {
        token->next = freedTokens;
        freedTokens = token;
    }
    else
    {
        delete token;
    }
}
const char* PdfToken::getValue() const
{
    return this->token;
}
PdfTokenType PdfToken::getType() const
{
    return this->type;
}
int PdfToken::getLen() const
{
    return this->token_len;
}
PdfParser::PdfParser(pdf_file_t* pdf, PdfParserReadType type, void* source)
{
    this->pdf = pdf;
    this->buffer = (unsigned char*)malloc(4096);
    this->buffer_size = 4096;
    this->splite_pos = NULL;
    this->current_pos = NULL;
    this->end_pos = NULL;
    this->remain.rem = NULL;
    this->remain.len = 0;
    this->freedTokens = NULL;
    this->reader.type = type;
    this->reader.source = source;
    switch (type)
    {
        case BUFFER_READER:
            this->reader.read = &PdfParser::_readBuffer;
            break;
        case FILE_READER:
            this->reader.read = &PdfParser::_readFile;
            break;
        case STREAM_READER:
            this->reader.read = &PdfParser::_readStream;
            break;
        default:
            printf("unknown reader type\n");
            break;
    }
}
PdfParser::~PdfParser()
{
    if (this->buffer)
    {
        free(this->buffer);
    }
    int last = this->end_pos - this->current_pos;
    if (last > 0)
    {
        fseek(this->pdf->pFile, -last, SEEK_CUR);
    }
    if (this->remain.len > 0)
    {
        free(this->remain.rem);
    }
    while (freedTokens != NULL)
    {
        PdfToken* t = freedTokens;
        freedTokens = freedTokens->next;
        delete t;
    }
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
void _decode_string(char** str, int* len)
{
    char* p = *str + 1; // skip '('
    char* end = *str + *len;
    char* out = (char*)calloc(1, *len * 2);
    int ol = 0;
    out[ol++] = **str; //'('
    while (p < end)
    {
        if (*p == '\\')
        {
            if (p + 1 >= end)
            {
                out[ol++] = 0x0;
                out[ol++] = *p;
                break;
            }
            p++;
            if (*p == 'n')
            {
                out[ol++] = 0x0;
                out[ol++] = '\n';
            }
            else if (*p == '\n')
            {
                // ignored
            }
            else if (*p == 'r')
            {
                out[ol++] = 0x0;
                out[ol++] = '\r';
            }
            else if (*p == 't')
            {
                out[ol++] = 0x0;
                out[ol++] = '\t';
            }
            else if (*p == 'b')
            {
                out[ol++] = 0x0;
                out[ol++] = '\b';
            }
            else if (*p == 'f')
            {
                out[ol++] = 0x0;
                out[ol++] = '\f';
            }
            else if (*p == '(' || *p == ')' || *p == '\\')
            {
                out[ol++] = 0x0;
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
            out[ol++] = 0x0;
            out[ol++] = *p;
        }
        p++;
    }
    free(*str);
    *str = out;
    *len = ol;
}

PdfToken* PdfParser::_getNextOneToken(const unsigned char* start, const unsigned char* end)
{
    if (start == NULL)
        return NULL;
    PdfToken* tk = NULL;
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
        
        auto to_compare = fixed_token_map[len].fixed_token_map;
        int operator_count = fixed_token_map[len].nums;
        bool isfind = false;
        int left = 0;
        int right = operator_count - 1;
        int mid = -1;
        while (left <= right)
        {
            mid = left + (right - left) / 2;
            int cmp = memcmp(start, to_compare[mid].token,  len);
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
            tk = _buildToken(start, to_compare[mid].type, len);
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

        tk = _buildToken(start, TOKEN_NAME, len);
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
        }

        tk = _buildToken(start, TOKEN_STRING, len);
        tk->token[tk->token_len - 1] = '\0';
        tk->token_len--;
        start += len;
        // decode string
        _decode_string(&tk->token, &tk->token_len);
    }
    else if (c == '[')
    {
        int len = 1;
        tk = _buildToken(start, TOKEN_ARRAY_BEG, len);
        start += len;
    }
    else if (c == ']')
    {
        int len = 1;
        tk = _buildToken(start, TOKEN_ARRAY_END, len);
        start += len;

    }
    else if (c == '<')
    {
        if (memcmp(start, "<<", 2) == 0) // dict
        {
            int len = 2;
            tk = _buildToken(start, TOKEN_DICT_BEG, len);
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

            tk = _buildToken(start, TOKEN_HEX_STRING, len);
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
            tk = _buildToken(start, TOKEN_DICT_END, len);
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
        tk = _buildToken(start, TOKEN_COMMENT, len);
        start += len;
    }
    else if (c == '-' || c == '+' || c == '.' || _is_digit(c))
    {
PARSE_NUMBER:
        tk = _buildNumber(start, end);
        start += tk->steps;
    }
    else if (_is_space(c))
    {
PARSE_SPACE:
        if (memcmp(start, "\r\n", 2) == 0)
        {
            tk = _buildToken(start, TOKEN_NEWLINE, 2);
            start += tk->steps;
        }
        else
        {
            tk = _buildToken(start, TOKEN_SPACE, 1);
            start += tk->steps;
        }
    }
    else
    {
        //printf("unknow token: %c(%#x)", *start, *start);
        return NULL;
    }

    return tk;
}
int PdfParser::_copyRem()
{
    int off = 0;
    if (this->splite_pos != NULL
        && this->current_pos != NULL
        && this->splite_pos >= this->current_pos)
    {
        int len = this->splite_pos - this->current_pos + 1;
        memcpy(this->buffer, this->current_pos, len);
        off += len;
    }
    memset(this->buffer + off, 0, this->buffer_size - off);
    this->current_pos = this->buffer;

    if (this->remain.len > 0)
    {
        memcpy(this->buffer + off, this->remain.rem, this->remain.len);
        off += this->remain.len;
        free(this->remain.rem);
        this->remain.rem = NULL;
        this->remain.len = 0;
    }

    return off;
}
void PdfParser::_split(int end_i)
{
    this->end_pos = this->buffer + end_i;

    while (end_i >= 0)
    {
        char c = this->buffer[end_i];
        if (_is_delimiter(c) || _is_space(c))
        {
            this->splite_pos = this->buffer + end_i;

            int len = this->end_pos - this->splite_pos;
            if (len > 0)
            {
                this->remain.rem = (unsigned char*)calloc(1, len + 1);
                this->remain.len = len;
                memcpy(this->remain.rem, this->buffer + end_i + 1, len);
                this->remain.rem[len] = '\0';
            }
            break;
        }
        end_i--;
    }
}
void PdfParser::_readFile(void* source)
{
    FILE* f = (FILE*)source;
    int off = _copyRem();
    int ret = 0;
    int need = this->buffer_size - off;
    ret = fread(this->buffer + off, 1, need, f);
    if (ret < need)
    {
        if (ferror(f))
            return;
    }
    int end_i = off + ret - 1;
    this->_split(end_i);
}
void PdfParser::_readBuffer(void* source)
{
    pdf_buffer_t* input = (pdf_buffer_t*)source;
    int off = this->_copyRem();
    int ret = 0;
    if (input != NULL)
    {
        if (input->processed < input->buffer_size)
        {
            ret = MIN(this->buffer_size - off, input->buffer_size - input->processed);
            memcpy(this->buffer + off, input->buffer + input->processed, ret);
            input->processed += ret;
        }
    }

    int end_i = off + ret - 1;
    this->_split(end_i);
}
void PdfParser::_readStream(void* source)
{
    pdf_stream_t* stream = (pdf_stream_t*)source;
    int off = _copyRem();
    int ret = 0;
    if (stream != NULL)
    {
        if ((stream->decomp.cur_pos >= stream->decomp.len && stream->readin_len < stream->stream_len)
            || stream->processed < stream->stream_len)
        {
            ret = pdf_stream_get_data(stream, this->buffer + off, this->buffer_size - off);
        }
    }

    int end_i = off + ret - 1;
    this->_split(end_i);
}

PdfToken* PdfParser::_getNextToken()
{
#if 1
    unsigned char** start = &(this->current_pos);
    unsigned char* end = this->splite_pos;

    while (*start < end && this->token_cache.size() < 3)
    {
        PdfToken* tk = _getNextOneToken(*start, end);
        if (tk == NULL)
            break;
        if (tk->type == TOKEN_SPACE || tk->type == TOKEN_COMMENT || tk->type == TOKEN_NEWLINE)
        {
            // ignored
            *start += tk->steps;
            delete tk;
            continue;
        }
        this->token_cache.push_back(tk);
        *start += tk->steps;
    }
    if (this->token_cache.size() == 0)
        return NULL;
    auto iter = this->token_cache.begin();
    PdfToken* tk = *iter; iter = this->token_cache.erase(iter);
    if (iter == this->token_cache.end())
    {
        return tk;
    }
    else if (tk->type == TOKEN_NUMBER)
    {
        if (this->token_cache.size() < 2)
        {
            return tk;
        }
        if (this->token_cache[0]->type != TOKEN_NUMBER)
        {
            return tk;
        }
        if (this->token_cache[1]->type == TOKEN_OBJ_BEG
            || this->token_cache[1]->type == TOKEN_INDIRECT)
        {
            PdfToken* token = new PdfToken;
            token->type = this->token_cache[1]->type;
            token->steps = tk->steps
                + this->token_cache[0]->steps
                + this->token_cache[1]->steps;
            token->token_len = tk->token_len
                + this->token_cache[0]->token_len
                + this->token_cache[1]->token_len
                + 2;// add 2 spaces
            token->token = (char*)malloc(token->token_len + 1);
            int off = 0;
            memcpy(token->token, tk->token, tk->token_len);
            off += tk->token_len;
            token->token[off] = ' ';
            off++;
            delete tk;

            memcpy(token->token + off, this->token_cache[0]->token, this->token_cache[0]->token_len);
            off += this->token_cache[0]->token_len;
            token->token[off] = ' ';
            off++;
            delete* iter;
            iter = this->token_cache.erase(iter);

            memcpy(token->token + off, this->token_cache[0]->token, this->token_cache[0]->token_len);
            off += this->token_cache[0]->token_len;
            token->token[off] = '\0';
            delete* iter;
            iter = this->token_cache.erase(iter);

            return token;
        }
    }
    return tk;
#else
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
#endif
}
PdfToken* pdf_stream_get_next_token(pdf_stream_t* stream)
{
    return stream->parser->getNextToken();
}

PdfToken* PdfParser::getNextToken()
{
    if (this->pdf == NULL)
        return NULL;
    if (this->pdf->pFile == NULL)
        return NULL;

    unsigned char* save_cur = this->current_pos;
    if (this->current_pos >= this->splite_pos)
    {
        (this->*(reader.read))(this->reader.source);
    }
    PdfToken* tk = _getNextToken();
    if (tk == NULL)
    {
        if (this->reader.type == STREAM_READER)
        {
            pdf_stream_t* s = (pdf_stream_t*)this->reader.source;
            if (s->processed < s->stream_len)
            {
                this->current_pos = save_cur;
                (this->*(reader.read))(this->reader.source);
                tk = _getNextToken();
            }
        }
    }
    return tk;
}
pdf_cmap_t* PdfParser::buildCMap()
{
    pdf_cmap_t* cmap = pdf_cmap_init();
    PdfToken* last_token = NULL;
    PdfToken* current_token = NULL;

    while ((current_token = getNextToken()) != NULL)
    {
        if (current_token->type == TOKEN_ENDCMAP)
        {
            delete last_token;
            last_token = NULL;
            delete current_token;
            current_token = NULL;
            break;
        }
        else if (current_token->type == TOKEN_BEGINBFCHAR || current_token->type == TOKEN_BEGINCIDCHAR)
        {
            // single char mapping
            bool isCid = (current_token->type == TOKEN_BEGINCIDCHAR);
            int unicode_map_len = strtol(last_token->token, NULL, 10);
            pdf_unicode_map_t* unicode_map = (pdf_unicode_map_t*)calloc(unicode_map_len, sizeof(pdf_unicode_map_t));
            delete last_token;
            last_token = NULL;
            delete current_token;
            current_token = NULL;

            for (int i = 0; i < unicode_map_len; i++)
            {
                last_token = getNextToken();
                current_token = getNextToken();
                unicode_map[i].cid = _hex_str_to_16bit(last_token->token + 1);
                if (isCid)
                {
                    unicode_map[i].unicode = (uint16_t)strtol(current_token->token + 1, NULL, 10);
                }
                else
                {
                    unicode_map[i].unicode = _hex_str_to_16bit(current_token->token + 1);
                }

                delete last_token;
                last_token = NULL;
                delete current_token;
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
            delete last_token;
            last_token = NULL;
            delete current_token;
            current_token = NULL;
            for (int i = 0; i < char_range_map_len; i++)
            {
                PdfToken* tk1 = getNextToken();
                PdfToken* tk2 = getNextToken();
                PdfToken* tk3 = getNextToken();
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
                delete tk1;
                delete tk2;
                delete tk3;
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
            delete last_token;
            last_token = NULL;
            delete current_token;
            current_token = NULL;
            for (int i = 0; i < code_range_map_len; i++)
            {
                PdfToken* tk1 = getNextToken();
                PdfToken* tk2 = getNextToken();
                code_range_map[i].srcStart = _hex_str_to_16bit(tk1->token + 1);
                code_range_map[i].srcEnd = _hex_str_to_16bit(tk2->token + 1);
                delete tk1;
                delete tk2;
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
            delete last_token;
            last_token = NULL;
        }
        last_token = current_token;
    }
    return cmap;
}
void PdfParser::_setCommonValue(PdfToken* tk, struct pdf_value* p)
{
    PdfTokenType type = tk->getType();
    if (type == TOKEN_NULL)
    {
        p->type = NUL;
    }
    else if (type == TOKEN_ARRAY_BEG)
    {
        p->type = ARRAY;
        p->val.array = buildArray();
    }
    else if (type == TOKEN_DICT_BEG)
    {
        p->type = DICT;
        p->val.dict = buildDict();
    }
    else if (type == TOKEN_BOOLEAN_TRUE)
    {
        p->type = BOOLEAN;
        p->val.boolean = true;
    }
    else if (type == TOKEN_BOOLEAN_FALSE)
    {
        p->type = BOOLEAN;
        p->val.boolean = false;
    }
    else if (type == TOKEN_NAME)
    {
        p->type = NAME;
        p->value_len = tk->getLen();
        p->val.name = (char*)malloc(p->value_len + 1);
        memcpy(p->val.name, tk->getValue(), p->value_len);
        p->val.name[p->value_len] = '\0';
    }
    else if (type == TOKEN_INDIRECT)
    {
        p->type = INDIRECT;
        char ref[256] = { 0 };
        memcpy(ref, tk->getValue(), tk->getLen());
        char* token = strtok(ref, " ");
        p->val.indirect = atoi(token);
    }
    else if (type == TOKEN_NUMBER)
    {
        p->type = NUMBER;
        p->val.number = strtod(tk->getValue(), NULL);
    }
    else if (type == TOKEN_STRING || type == TOKEN_HEX_STRING)
    {
        p->type = STRING;
        p->value_len = tk->getLen();
        p->val.string = (char*)malloc(p->value_len + 1);
        memcpy(p->val.string, tk->getValue(), p->value_len);
        p->val.string[p->value_len] = '\0';
    }
}
pdf_obj_t* PdfParser::buildObj()
{
    PdfToken* tk;
    pdf_obj_t* obj = pdf_obj_init();
    obj->value = (pdf_value*)malloc(sizeof(pdf_value));

    while ((tk = getNextToken()) != NULL)
    {
        if (tk->type == TOKEN_OBJ_END)
        {
            delete tk;
            break;
        }
        else if (tk->type == TOKEN_STREAM_BEG)
        {
            fseek(this->pdf->pFile, -(this->end_pos - this->current_pos + 1), SEEK_CUR);
            int offset = ftell(this->pdf->pFile);
            char c;
            while (true)
            {
                c = getc(this->pdf->pFile);
                if (c != '\r' && c != '\n')
                {
                    fseek(this->pdf->pFile, -1, SEEK_CUR);
                    break;
                }
            }
            // store current offset
            offset = ftell(this->pdf->pFile);
            int len = pdf_dict_get_number(obj->value->val.dict, "/Length");
            if (len == -1)
            {
                int ref = pdf_dict_get_ref(obj->value->val.dict, "/Length");
                pdf_obj_t* l_obj = pdf_file_get_obj(this->pdf, ref);
                len = l_obj->value->val.number;
                fseek(this->pdf->pFile, offset, SEEK_SET);
            }
            //int offset = ftell(parser->pdf->pFile);
            obj->stream = pdf_stream_init(this->pdf, obj, len, offset);
            fseek(this->pdf->pFile, len, SEEK_CUR);
            free(this->remain.rem);
            this->remain.rem = 0;
            this->remain.len = 0;
            this->splite_pos = NULL;
            this->current_pos = NULL;
            (this->*(reader.read))(this->reader.source);
            delete tk;

            tk = getNextToken();
            if (tk == NULL || tk->type != TOKEN_STREAM_END)
            {
                pdf_obj_free(obj);
                return NULL;
            }
        }
        else
        {
            _setCommonValue(tk, obj->value);
        }

        delete tk;
    }

    return obj;
}
PdfDict* PdfParser::buildDict()
{
    // pdf_dict_pair_t* p = NULL;
    // pdf_dict_pair_t** head = NULL;
    PdfDict* dict = new PdfDict;
    PdfToken* tk;

    while ((tk = getNextToken()) != NULL)
    {
        if (tk->type == TOKEN_DICT_END)
        {
            delete tk;
            break;
        }
        PdfToken* tk1 = getNextToken();
        if (tk1 == NULL)
        {
            break;
        }
        pdf_value* v = (pdf_value*)calloc(1, sizeof(pdf_value));
        _setCommonValue(tk1, v);
        (*dict)[tk->token] = v;
        delete tk;
        delete tk1;
    }
    return dict;
}
PdfArray* PdfParser::buildArray()
{
    //pdf_array_element_value_t** head = NULL;
    PdfArray* array = new PdfArray;
    PdfToken* tk;

    while ((tk = getNextToken()) != NULL)
    {
        if (tk->type == TOKEN_ARRAY_END)
        {
            delete tk;
            break;
        }
        else
        {
            pdf_value* v = (pdf_value*)calloc(1, sizeof(pdf_value));
            _setCommonValue(tk, v);
            array->push_back(v);
        }

        delete tk;
    }

    return array;
}