#include "pdf-private.h"
#include <stdlib.h>

// void pdf_value_free(struct pdf_value* value)
// {
//     if (value == NULL) return;
//     enum pdf_value_type type = value->type;
//     if (type == NAME)
//     {
//         free(value->name);
//         value->name = NULL;
//     }
//     else if (type == STRING)
//     {
//         free(value->string);
//         value->string = NULL;
//     }
//     else if (type == DICT)
//     {
//         delete value->dict;
//         value->dict = NULL;
//     }
//     else if (type == ARRAY)
//     {
//         delete value->array;
//         value->array = NULL;
//     }
//     free(value);
//     value = NULL;
// }
PdfValue::~PdfValue()
{
    if (type == NAME)
    {
        free(name);
    }
    else if (type == STRING)
    {
        free(string);
    }
    else if (type == DICT)
    {
        delete dict;
    }
    else if (type == ARRAY)
    {
        delete array;
    }
}