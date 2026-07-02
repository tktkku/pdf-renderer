#include "pdf.h"
#include "pdf-private.h"


pdf_dict::pdf_dict()
{
}

pdf_dict::~pdf_dict()
{
    for (auto it = pairs.begin(); it != pairs.end(); it++)
    {
        pdf_value_free(it->second);
        it->second = NULL;
    }
    pairs.clear();
}

pdf_indirect_t pdf_dict::get_indirect(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_INDIRECT)
    {
        return it->second->val.indirect;
    }
    return { -1, -1 };
}

bool pdf_dict::is_indirect(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_INDIRECT;
    }
    return false;
}

int pdf_dict::get_boolean(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_BOOLEAN)
    {
        return it->second->val.boolean;
    }
    return -1;
}

bool pdf_dict::is_boolean(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_BOOLEAN;
    }
    return false;
}

double pdf_dict::get_number(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_NUMBER)
    {
        return it->second->val.number;
    }
    return -1;
}

bool pdf_dict::is_number(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_NUMBER;
    }
    return false;
}

char* pdf_dict::get_string(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_STRING)
    {
        return it->second->val.string;
    }
    return NULL;
}

bool pdf_dict::is_string(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_STRING;
    }
    return false;
}

char* pdf_dict::get_name(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_NAME)
    {
        return it->second->val.name;
    }
    return NULL;
}

bool pdf_dict::is_name(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_NAME;
    }
    return false;
}

pdf_dict* pdf_dict::get_dict(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_DICT)
    {
        return it->second->val.dict;
    }
    return NULL;
}
bool pdf_dict::is_dict(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_DICT;
    }
    return false;
}

pdf_array* pdf_dict::get_array(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end() && it->second->type == PDF_VALUE_ARRAY)
    {
        return it->second->val.array;
    }
    return NULL;
}

bool pdf_dict::is_array(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second->type == PDF_VALUE_ARRAY;
    }
    return false;
}

bool pdf_dict::has(const char* key)
{
    return pairs.find(key) != pairs.end();
}

pdf_value_t* pdf_dict::operator[](const char* key)
{
    return get(key);
}

pdf_value_t* pdf_dict::get(const char* key)
{
    auto it = pairs.find(key);
    if (it != pairs.end())
    {
        return it->second;
    }
    return NULL;
}

void pdf_dict::add(const char* key, pdf_value_type_t type, void* data)
{
    pdf_value_t* value = pdf_value_init();
    value->type = type;
    switch (type)
    {
        case PDF_VALUE_BOOLEAN:
            value->val.boolean = *((bool*)data);
            break;
        case PDF_VALUE_NAME:
            {
                int len = strlen((char*)data);
                value->val.name = (char*)malloc(len + 1);
                memcpy(value->val.name, data, len);
                value->val.name[len] = '\0';
            }
            break;
        case PDF_VALUE_INDIRECT:
            value->val.indirect = *((pdf_indirect_t*)data);
            break;
        case PDF_VALUE_NUMBER:
            value->val.number = *((double*)data);
            break;
        case PDF_VALUE_ARRAY:
            value->val.array = (pdf_array*)data;
            break;
        case PDF_VALUE_DICT:
            value->val.dict = (pdf_dict*)data;
            break;
        case PDF_VALUE_STRING:
            {
                int len = strlen((char*)data);
                value->val.string = (char*)malloc(len + 1);
                memcpy(value->val.string, data, len);
                value->val.string[len] = '\0';
            }
            break;
        default:
            free(value);
            return;
    }
    add(key, value);
}

void pdf_dict::add(const char* key, pdf_value_t* value)
{
    pairs[key] = value;
}