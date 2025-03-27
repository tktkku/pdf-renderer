#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>

void pdf_array_free(PdfArray* array)
{
    if (array == NULL)
    {
        return;
    }

    if (array->size() > 0)
    {
        for (auto* ptr : *array)
        {
            pdf_value_free(ptr);
        }
    }
    delete array;
}