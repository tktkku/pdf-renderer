#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

pdf_stack_t* pdf_stack_init()
{
    pdf_stack_t* s = (pdf_stack_t*)malloc(sizeof(pdf_stack_t));
    memset(s, 0, sizeof(pdf_stack_t));

    return s;
}

void pdf_stack_free(pdf_stack_t* s)
{
    if (s == NULL) return;

    pdf_stack_node_t* node;
    pdf_stack_node_t* node_p;
    for (node = s->top; node != NULL; )
    {
        node_p = node->next;
        free(node->data);
        free(node);
        node = node_p;
    }
    
    s->top = NULL;
    free(s);
}

void pdf_stack_push(pdf_stack_t* s, const void* data, size_t size)
{
    if (s == NULL) return;
    pdf_stack_node_t* node = (pdf_stack_node_t*)malloc(sizeof(pdf_stack_node_t));
    memset(node, 0, sizeof(pdf_stack_node_t));
    node->data = (char*)malloc(size + 1);
    memcpy(node->data, data, size);
    node->data[size] = '\0';
    node->size = size;
    
    if (s->top == NULL)
    {
        node->next = NULL;
        s->top = node;
    }
    else
    {
        node->next = s->top;
        s->top = node;
    }
}

void pdf_stack_show(pdf_stack_t* s)
{
    if (s == NULL) return;
    pdf_stack_node_t *node = s->top;
    printf("######\n");
    for ( ; node; node = node->next)
    {
        printf("%s(", (char*)node->data);
        for (int i = 0; i < node->size; i++)
        {
            printf("%#x ", *(char*)(node->data + i));
        }
        printf(")\n");
    }
    printf("######\n");
}

void pdf_stack_pop(pdf_stack_t* s, pdf_stack_node_t* data)
{
    if (s == NULL || s->top == NULL)
    {
        return;
    }

    pdf_stack_node_t* node = s->top;

    data->size = node->size;
    memcpy(data->data, node->data, node->size);
    data->data[node->size] = '\0';
    s->top = node->next;

    free(node->data);
    free(node);
}