#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>

pdf_dict_t* pdf_dict_init()
{
    pdf_dict_t* dict = new pdf_dict_t;
    if (dict == NULL) 
        return NULL;
    return dict;
}
void pdf_dict_free(pdf_dict_t* dict)
{
    if (dict == NULL)
    {
        return;
    }
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (dict->pairs[i]->name)
        {
            free(dict->pairs[i]->name);
            dict->pairs[i]->name = NULL;
        }
        pdf_value_free(dict->pairs[i]->value);
        dict->pairs[i]->value = NULL;
        free(dict->pairs[i]);
        //dict->pairs[i] = NULL;
    }
    dict->pairs.clear();
    delete dict;
    dict = NULL;
}

double pdf_dict_get_number(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return 0.0;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_NUMBER)
            {
                return dict->pairs[i]->value->val.number;
            }
        }
    }

    return -1.0;
}

int pdf_dict_get_ref(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return -1;

    if (dict->pairs.empty()) return -1;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_INDIRECT)
            {
                return dict->pairs[i]->value->val.indirect;
            }
        }
    }

    return -1;
}

pdf_array_t* pdf_dict_get_array(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return 0;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_ARRAY)
            {
                return dict->pairs[i]->value->val.array;
            }
        }
    }

    return NULL;
}

pdf_dict_t* pdf_dict_get_dict(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return NULL;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_DICT)
            {
                return dict->pairs[i]->value->val.dict;
            }
        }
    }

    return NULL;
}

char* pdf_dict_get_name(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return NULL;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_NAME)
            {
                return dict->pairs[i]->value->val.name;
            }
        }
    }

    return NULL;
}

char* pdf_dict_get_string(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return NULL;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_STRING)
            {
                return dict->pairs[i]->value->val.string;
            }
        }
    }

    return NULL;
}

int pdf_dict_get_bool(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return -1;
    int nums = dict->pairs.size();
    for (int i = 0; i < nums; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == PDF_VALUE_BOOLEAN)
            {
                return dict->pairs[i]->value->val.boolean;
            }
        }
    }

    return -1;
}

bool pdf_dict_add_array(pdf_dict_t* dict, const char* name, pdf_array_t* array)
{
    if (dict == NULL || name == NULL) return false;

    pdf_dict_pair_t* p = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
    int len = strlen(name);
    p->name = (char*)malloc(len + 1);
    memcpy(p->name, name, len);
    p->name[len] = '\0';
    p->name_len = len;
    p->value = (pdf_dict_pair_value_t*)malloc(sizeof(pdf_dict_pair_value_t));
    p->value->type = PDF_VALUE_ARRAY;
    p->value->val.array = array;

    dict->pairs.push_back(p);
    return true;
}
bool pdf_dict_add_value(pdf_dict_t* dict, const char* name, pdf_value_t* value)
{
    if (dict == NULL || name == NULL || value == NULL) return false;
    pdf_dict_pair_t* p = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
    int len = strlen(name);
    p->name = (char*)malloc(len + 1);
    memcpy(p->name, name, len);
    p->name[len] = '\0';
    p->name_len = len;
    p->value = value;
    dict->pairs.push_back(p);
    return true;
}

bool pdf_dict_add(pdf_dict_t* dict, const char* name, pdf_value_type_t type, void* data)
{
    if (dict == NULL || name == NULL || data == NULL) return false;

    pdf_dict_pair_t* p = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
    int len = strlen(name);
    p->name = (char*)malloc(len + 1);
    memcpy(p->name, name, len);
    p->name[len] = '\0';
    p->name_len = len;
    p->value = (pdf_dict_pair_value_t*)malloc(sizeof(pdf_dict_pair_value_t));
    p->value->type = type;
    switch (type)
    {
        case PDF_VALUE_BOOLEAN:
            p->value->val.boolean = *((bool*)data);
            break;
        case PDF_VALUE_NAME:
            {
                len = strlen((char*)data);
                p->value->val.name = (char*)malloc(len + 1);
                memcpy(p->value->val.name, data, len);
                p->value->val.name[len] = '\0';
            }
            break;
        case PDF_VALUE_INDIRECT:
            p->value->val.indirect = *((int*)data);
            break;
        case PDF_VALUE_NUMBER:
            p->value->val.number = *((double*)data);
            break;
        case PDF_VALUE_ARRAY:
            p->value->val.array = (pdf_array_t*)data;
            break;
        case PDF_VALUE_DICT:
            p->value->val.dict = (pdf_dict_t*)data;
            break;
        case PDF_VALUE_STRING:
            {
                len = strlen((char*)data);
                p->value->val.string = (char*)malloc(len + 1);
                memcpy(p->value->val.string, data, len);
                p->value->val.string[len] = '\0';
            }
            break;
        default:
        {
            pdf_value_free(p->value);
            free(p->name);
            free(p);
            return false;
        }
    }
    
    dict->pairs.push_back(p);
    return true;
}