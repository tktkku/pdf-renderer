#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>
PdfDict::~PdfDict()
{
    for (auto& pair : entries)
    {
        delete pair.second;
    }
}
PdfValue& PdfDict::operator[](const std::string& key)
{
    auto it = entries.find(key);
    if (it == entries.end())
    {
        PdfValue* v = new PdfValue;
        entries[key] = v;
        return *v;
    }
    return *(it->second);
}
// void pdf_dict_free(PdfDict* dict)
// {
//     if (dict == NULL)
//     {
//         return;
//     }

//     if (dict->size() > 0)
//     {
//         for (auto& pair : *dict)
//         {
//             pdf_value_free(pair.second);
//         }
//     }
//     delete dict;
// }

// double pdf_dict_get_number(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL) return -1.0;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return -1.0;
//     }
//     else
//     {
//         if (it->second->type == NUMBER)
//             return it->second->val.number;
//         else
//             return -1;
//     }
// }

// int pdf_dict_get_ref(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL) return -1;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return -1;
//     }
//     else
//     {
//         if (it->second->type == INDIRECT)
//             return it->second->val.indirect;
//         else
//             return -1;
//     }
// }

// PdfArray* pdf_dict_get_array(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL) return 0;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return NULL;
//     }
//     else
//     {
//         if (it->second->type == ARRAY)
//             return it->second->val.array;
//         else
//             return NULL;
//     }
// }

// PdfDict* pdf_dict_get_dict(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL ) return NULL;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return NULL;
//     }
//     else
//     {
//         if (it->second->type == DICT)
//             return it->second->val.dict;
//         else
//             return NULL;
//     }
// }

// const char* pdf_dict_get_name(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL ) return NULL;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return NULL;
//     }
//     else
//     {
//         if (it->second->type == NAME)
//             return it->second->val.name;
//         else 
//             return NULL;
//     }
// }

// const char* pdf_dict_get_string(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL ) return NULL;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return NULL;
//     }
//     else
//     {
//         if (it->second->type == STRING)
//             return it->second->val.string;
//         else
//             return NULL;
//     }
// }

// int pdf_dict_get_bool(PdfDict* dict, const char* name)
// {
//     if (dict == NULL || name == NULL ) return -1;
//     auto it = dict->find(name);
//     if (it == dict->end())
//     {
//         return -1;
//     }
//     else
//     {
//         if (it->second->type == BOOLEAN)
//             return it->second->val.boolean;
//         else
//             return -1;
//     }
// }

// bool pdf_dict_add_array(PdfDict* dict, const char* name, PdfArray* array)
// {
//     if (dict == NULL || name == NULL) return false;
//     pdf_value* v = (pdf_value*)calloc(1, sizeof(pdf_value));
//     v->type = ARRAY;
//     v->val.array = array;
//     (*dict)[name] = v;

//     return true;
// }