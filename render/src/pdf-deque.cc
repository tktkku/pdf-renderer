#include "pdf.h"
#include "pdf-private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdexcept>
// just reference the data pointer, do not copy the data
pdf_node::pdf_node(const void* data, size_t size)
{
    if (data != NULL && size > 0)
    {
        this->_data.insert(this->_data.end(), (const char*)data, (const char*)data + size);
    }
    this->_data.push_back('\0');
    this->next = NULL;
    this->prev = NULL;
}
pdf_node::pdf_node(pdf_node* other)
{
    if (other != NULL)
    {
        this->_data.insert(this->_data.end(), other->data(), other->data() + other->size());
    }
    this->_data.push_back('\0');
    this->next = NULL;
    this->prev = NULL;
}
pdf_node::~pdf_node()
{

}
char* pdf_node::data()
{
    return this->_data.data();
}
const char pdf_node::operator[](size_t index) const
{
    if (index >= this->_data.size() - 1)
    {
        throw std::out_of_range("Index out of range");
    }
    return this->_data[index];
}
size_t pdf_node::size()
{
    return this->_data.size() - 1;
}
pdf_node* pdf_node::get_next()
{
    return this->next;
}
pdf_node* pdf_node::get_prev()
{
    return this->prev;
}
void pdf_node::set_next(pdf_node* next)
{
    this->next = next;
}
void pdf_node::set_prev(pdf_node* prev)
{
    this->prev = prev;
}
// front [1]--[2]--[3] end
pdf_deque::pdf_deque()
{
    this->front = this->rear = NULL;
    this->_size = 0;
}

pdf_deque::~pdf_deque()
{
    pdf_node* cur = this->front;
    while (cur)
    {
        pdf_node* next = cur->get_next();
        delete cur;
        cur = next;
    }
}
bool pdf_deque::empty()
{
    return this->front == NULL;
}

void pdf_deque::clear()
{
    pdf_node* cur = this->front;
    while (cur)
    {
        pdf_node* next = cur->get_next();
        delete cur;
        cur = next;
    }
    this->front = this->rear = NULL;
    this->_size = 0;
}

void pdf_deque::push_back(const void* data, size_t size)
{
    pdf_node* node = new pdf_node(data, size);
    node->set_next(NULL);
    node->set_prev(NULL);
    if (this->front == NULL)
    {
        this->front = this->rear = node;
        this->_size = 1;
    }
    else
    {
        // [1]--[2]--[3]
        //      end

        this->rear->set_next(node);
        node->set_prev(this->rear);

        this->rear = node;
        this->_size++;
    }
}
void pdf_deque::push_front(const void* data, size_t size)
{
    pdf_node* node = new pdf_node(data, size);
    node->set_next(NULL);
    node->set_prev(NULL);
    if (this->rear == NULL)
    {
        this->front = this->rear = node;
        this->_size = 1;
    }
    else
    {
        // [1]--[2]--[3]
        //      front
        this->front->set_prev(node);
        node->set_next(this->front);
        this->front = node;
        this->_size++;
    }
}
pdf_node& pdf_deque::get(int index)
{
    if (this->front == NULL)
    {
        throw std::out_of_range("Index out of range");
    }
    // [3]--[2]--[1]
    //            front
    pdf_node* cur = this->front;
    int cnt;
    for (cnt = 0; cur && cnt < index; cnt++)
    {
        cur = cur->get_next();
    }
    if (cur == NULL)
    {
        throw std::out_of_range("Index out of range");
    }
    else
    {
        return *cur;
    }
}
std::unique_ptr<pdf_node> pdf_deque::pop_front()
{
    if (this->front == NULL)
    {
        return std::unique_ptr<pdf_node>(nullptr);
    }
    // 
    // [1]--[2]--[3]
    // node next
    pdf_node* node = this->front;
    pdf_node* next = node->get_next();
    if (next != NULL)
    {
        next->set_prev(NULL);
        this->front = next;
    }
    else
    {
        this->front = this->rear = NULL;
    }
    
    this->_size--;
    std::unique_ptr<pdf_node> ret(node);
    return ret;
}

std::unique_ptr<pdf_node> pdf_deque::pop_back()
{
    if (this->front == NULL || this->rear == NULL)
    {
        return std::unique_ptr<pdf_node>(nullptr);
    }
    // [1]--[2]--[3]
    //      prev node
    pdf_node* node = this->rear;
    pdf_node* prev = node->get_prev();
    if (prev != NULL)
    {
        prev->set_next(NULL);
        this->rear = prev;
    }
    else
    {
        this->front = this->rear = NULL;
    }
    
    this->_size--;
    std::unique_ptr<pdf_node> ret(node);
    return ret;
}

size_t pdf_deque::size()
{
    return this->_size;
}