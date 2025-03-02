#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>

pdf_dict_t* pdf_dict_init()
{
    pdf_dict_t* dict = (pdf_dict_t*)malloc(sizeof(pdf_dict_t));
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

    if (dict->pairs)
    {
        for (int i = 0; i < dict->num_pairs; i++)
        {
            if (dict->pairs[i]->name)
            {
                free(dict->pairs[i]->name);
                dict->pairs[i]->name = NULL;
            }
            pdf_value_free(dict->pairs[i]->value);
            dict->pairs[i]->value = NULL;
            free(dict->pairs[i]);
            dict->pairs[i] = NULL;
        }
        free(dict->pairs);
        dict->pairs = NULL;
    }

    free(dict);
    dict = NULL;
}

double pdf_dict_get_number(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL) return 0.0;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == NUMBER)
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

    if (dict->pairs == NULL) return -1;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == INDIRECT)
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

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == ARRAY)
            {
                return dict->pairs[i]->value->val.array;
            }
        }
    }

    return NULL;
}

pdf_dict_t* pdf_dict_get_dict(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL ) return NULL;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == DICT)
            {
                return dict->pairs[i]->value->val.dict;
            }
        }
    }
    
    return NULL;
}

const char* pdf_dict_get_name(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL ) return NULL;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == NAME)
            {
                return dict->pairs[i]->value->val.name;
            }
        }
    }

    return NULL;
}

const char* pdf_dict_get_string(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL ) return NULL;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == STRING)
            {
                return dict->pairs[i]->value->val.string;
            }
        }
    }

    return NULL;
}

int pdf_dict_get_bool(pdf_dict_t* dict, const char* name)
{
    if (dict == NULL || name == NULL ) return -1;

    for (int i = 0; i < dict->num_pairs; i++)
    {
        if (strcmp(dict->pairs[i]->name, name) == 0)
        {
            if (dict->pairs[i]->value->type == BOOLEAN)
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

    pdf_dict_pair_t** t = (pdf_dict_pair_t**)realloc(dict->pairs, (dict->num_pairs + 1) * sizeof(pdf_dict_pair_t*));
    if (t == NULL)
    {
        return false;
    }
    dict->pairs = t;

    pdf_dict_pair_t* p = (pdf_dict_pair_t*)malloc(sizeof(pdf_dict_pair_t));
    int len = strlen(name);
    p->name = (char*)malloc(len + 1);
    memcpy(p->name, name, len);
    p->name[len] = '\0';
    p->name_len = len;
    p->value = (pdf_dict_pair_value_t*)malloc(sizeof(pdf_dict_pair_value_t));
    p->value->type = ARRAY;
    p->value->val.array = array;

    dict->pairs[dict->num_pairs] = p;
    dict->num_pairs++;

    return true;
}