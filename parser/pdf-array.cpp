#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
PdfArray::~PdfArray()
{
    for (auto* ptr : elements)
    {
        delete ptr;
    }
}
PdfValue* PdfArray::operator[](int index) const
{
    if (index >= elements.size()) return NULL;
    return elements[index];
}
// void pdf_array_free(PdfArray* array)
// {
//     if (array == NULL)
//     {
//         return;
//     }

//     if (array->size() > 0)
//     {
//         for (auto* ptr : *array)
//         {
//             pdf_value_free(ptr);
//         }
//     }
//     delete array;
// }