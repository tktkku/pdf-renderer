#include "pdf-private.h"
#include <stdlib.h>

void pdf_value_free(struct pdf_value* value)
{
    if (value == NULL) return;
    enum pdf_value_type type = value->type;
    if (type == PDF_VALUE_NAME)
    {
        free(value->val.name);
        value->val.name = NULL;
    }
    else if (type == PDF_VALUE_STRING)
    {
        free(value->val.string);
        value->val.string = NULL;
    }
    else if (type == PDF_VALUE_DICT)
    {
        delete value->val.dict;
        value->val.dict = NULL;
    }
    else if (type == PDF_VALUE_ARRAY)
    {
        delete value->val.array;
        value->val.array = NULL;
    }
    free(value);
    value = NULL;
}