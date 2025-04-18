#include "pdf-private.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
// front [1]--[2]--[3] end

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

void pdf_deque_empty(pdf_deque_t* deque)
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
    deque->front = deque->rear = NULL;
    deque->size = 0;
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
    node->prev = NULL;
    if (q->front == NULL)
    {
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
    pdf_node_t* next = node->next;
    if (next != NULL)
    {
        next->prev = NULL;
        q->front = next;
    }
    else
    {
        q->front = q->rear = NULL;
    }
    
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
    //      prev node
    pdf_node_t* node = q->rear;
    pdf_node_t* prev = node->prev;
    if (prev != NULL)
    {
        prev->next = NULL;
        q->rear = prev;
    }
    else
    {
        q->front = q->rear = NULL;
    }
    
    q->size--;

    data->size = node->size;
    memcpy(data->data, node->data, node->size);
    ((char*)data->data)[node->size] = '\0';

    free(node->data);
    free(node);
}