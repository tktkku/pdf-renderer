#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdio.h>

pdf_obj_t* pdf_obj_init()
{
    pdf_obj_t* obj = (pdf_obj_t*)malloc(sizeof(pdf_obj_t));
    memset(obj, 0, sizeof(pdf_obj_t));

    return obj;
}

void pdf_obj_free(pdf_obj_t* obj)
{
    if (obj == NULL)
    {
        return;
    }
    if (obj->stream != NULL)
    {
        pdf_stream_free(obj->stream);
    }
    if (obj->xobject != NULL)
    {
        pdf_page_xobject_free(obj->xobject);
    }
    if (obj->font_data != NULL)
    {
        free(obj->font_data);
    }
    pdf_value_free(obj->value);
    obj->value = NULL;
    free(obj);
    obj = NULL;
}