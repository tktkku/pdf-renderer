#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

pdf_deque_t* pdf_deque_init()
{
    pdf_deque_t* q = (pdf_deque_t*)malloc(sizeof(pdf_deque_t));
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

void pdf_deque_free(pdf_deque_t* deque)
{
    if (deque == NULL) return;
    pdf_node_t* cur = deque->front;
    while (cur)
    {
        pdf_node_t* next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    free(deque);
}

void pdf_deque_push(pdf_deque_t* q, const void* data, size_t size)
{
    if (q == NULL) return;
    pdf_node_t* node = (pdf_node_t*)malloc(sizeof(pdf_node_t));
    node->data = malloc(size + 1);
    memcpy(node->data, data, size);
    ((char*)node->data)[size] = '\0';
    node->size = size;
    node->next = NULL;

    if (q->front == NULL)
    {
        node->prev = NULL;
        q->front = q->rear = node;
        q->size = 1;
    }
    else
    {
        // [1]--[2]--[3]
        //      front
        q->front->prev = node;
        node->next = q->front;

        q->front = node;
        q->size++;
    }
}

void pdf_deque_pop_front(pdf_deque_t* q, pdf_node_t* data)
{
    if (q == NULL || q->front == NULL)
    {
        data->size = 0;
        ((char*)data->data)[0] = '\0';
        return;
    }
    // 
    // [1]--[2]--[3]
    // node next
    pdf_node_t* node = q->front;
    pdf_node_t* next = q->front->next;
    if (next != NULL) next->prev = NULL;
    q->front = next;
    q->size--;

    data->size = node->size;
    memcpy(data->data, node->data, node->size);
    ((char*)data->data)[node->size] = '\0';

    free(node->data);
    free(node);
}

void pdf_deque_pop_end(pdf_deque_t* q, pdf_node_t* data)
{
    if (q == NULL || q->front == NULL || q->rear == NULL)
    {
        data->size = 0;
        ((char*)data->data)[0] = '\0';
        return;
    }
    // [1]--[2]--[3]
    //      prev rear
    pdf_node_t* prev = q->rear->prev;
    pdf_node_t* node = q->rear;
    if (prev != NULL) prev->next = NULL;
    q->rear = prev;
    q->size--;

    data->size = node->size;
    memcpy(data->data, node->data, node->size);
    ((char*)data->data)[node->size] = '\0';

    free(node->data);
    free(node);
}