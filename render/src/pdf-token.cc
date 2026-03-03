#include "pdf.h"
#include "pdf-private.h"


pdf_token::pdf_token(pdf_token_type_t type)
{
    _type = type;
    _data.push_back('\0');
}

pdf_token::pdf_token(const char* start, int len, pdf_token_type_t type)
{
    _type = type;
    _steps = len;
    if (start != NULL)
    {
        _data.insert(_data.end(), start, start + len);
    }
    
    if (type == TOKEN_NAME)
    {
        decode_name();
    }
    else if (type == TOKEN_STRING)
    {
        decode_string();
    }
    else if (type == TOKEN_HEX_STRING)
    {
        decode_hex_string();
    }
    _data.push_back('\0');
}

pdf_token::pdf_token(const char* start, int len, pdf_token_type_t type, int steps)
{
    _type = type;
    _steps = steps;
    if (start != NULL)
    {
        _data.insert(_data.end(), start, start + len);
    }
    
    if (type == TOKEN_NAME)
    {
        decode_name();
    }
    else if (type == TOKEN_STRING)
    {
        decode_string();
    }
    else if (type == TOKEN_HEX_STRING)
    {
        decode_hex_string();
    }
    _data.push_back('\0');
}

void pdf_token::append(pdf_token* other)
{
    _data.pop_back();
    _data.insert(_data.end(), other->_data.begin(), other->_data.end() - 1);
    _data.push_back('\0');
    _steps += other->steps();
}
void pdf_token::append(const char* data, int len)
{
    _data.pop_back();
    _data.insert(_data.end(), data, data + len);
    _data.push_back('\0');
}
const char* pdf_token::data()
{
    return _data.data();
}
pdf_token_type_t pdf_token::type()
{
    return _type;
}
size_t pdf_token::size()
{
    return _data.size() - 1;
}
size_t pdf_token::steps()
{
    return _steps;
}
bool pdf_token::empty()
{
    return size() == 0;
}