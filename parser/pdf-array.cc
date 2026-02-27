#include "pdf.h"
#include "pdf-private.h"

pdf_array::pdf_array() {}
pdf_array::~pdf_array()
{
    for (size_t i = 0; i < elements.size(); i++)
    {
        pdf_value_free(elements[i]);
        elements[i] = NULL;
    }
    elements.clear();
}

pdf_value_t* pdf_array::operator[](size_t index)
{
    return get(index);
}

pdf_value_t* pdf_array::get(size_t index)
{
    if (index >= elements.size())
    {
        return NULL;
    }
    return elements[index];
}
void pdf_array::add(pdf_value_t* value)
{
    elements.push_back(value);
}

size_t pdf_array::size()
{
    return elements.size();
}