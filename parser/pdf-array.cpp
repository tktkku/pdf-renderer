#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
pdf_array_t* pdf_array_init()
{
    pdf_array_t* array = (pdf_array_t*)malloc(sizeof(pdf_array_t));
    memset(array, 0, sizeof(pdf_array_t));

    return array;
}

void pdf_array_free(pdf_array_t* array)
{
    if (array == NULL)
    {
        return;
    }

    if (array->values)
    {
        for (int i = 0; i < array->num_elements; i++)
        {
            pdf_array_element_value_t* v = array->values[i];
            pdf_value_free(v);
        }
        free(array->values);
        array->values = NULL;
    }

    free(array);
    array = NULL;
}