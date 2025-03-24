#include "pdf-private.h"
#include <stdlib.h>

void pdf_value_free(struct pdf_value* value)
{
    if (value == NULL) return;
    enum pdf_value_type type = value->type;
    if (type == NAME)
    {
        free(value->val.name);
        value->val.name = NULL;
    }
    else if (type == STRING)
    {
        free(value->val.string);
        value->val.string = NULL;
    }
    else if (type == DICT)
    {
        pdf_dict_free(value->val.dict);
        value->val.dict = NULL;
    }
    else if (type == ARRAY)
    {
        pdf_array_free(value->val.array);
        value->val.array = NULL;
    }
    free(value);
    value = NULL;
}