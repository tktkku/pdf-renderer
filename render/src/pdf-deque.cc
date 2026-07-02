#include "pdf.h"
#include "pdf-private.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdexcept>

// ponytail: std::vector-based deque replaces linked list — O(1) get() for TJ arrays
// Stack top = vector back; push_front → push_back; pop_front → pop_back
pdf_node::pdf_node(const void* data, size_t size)
{
    this->_size = (data != NULL && size > 0) ? size : 0;
    this->_is_small = (this->_size < SMALL_BUF_SIZE);
    if (this->_is_small)
    {
        if (this->_size > 0)
            memcpy(this->_small_buf, data, this->_size);
        this->_small_buf[this->_size] = '\0';
        this->_data_ptr = this->_small_buf;
    }
    else
    {
        this->_data_ptr = (char*)malloc(this->_size + 1);
        if (this->_size > 0)
            memcpy(this->_data_ptr, data, this->_size);
        this->_data_ptr[this->_size] = '\0';
    }
    this->next = NULL;
    this->prev = NULL;
}
pdf_node::pdf_node(pdf_node* other)
{
    if (other != NULL)
    {
        this->_size = other->_size;
        this->_is_small = (this->_size < SMALL_BUF_SIZE);
        if (this->_is_small)
        {
            if (this->_size > 0)
                memcpy(this->_small_buf, other->data(), this->_size);
            this->_small_buf[this->_size] = '\0';
            this->_data_ptr = this->_small_buf;
        }
        else
        {
            this->_data_ptr = (char*)malloc(this->_size + 1);
            if (this->_size > 0)
                memcpy(this->_data_ptr, other->data(), this->_size);
            this->_data_ptr[this->_size] = '\0';
        }
    }
    else
    {
        this->_size = 0;
        this->_is_small = true;
        this->_small_buf[0] = '\0';
        this->_data_ptr = this->_small_buf;
    }
    this->next = NULL;
    this->prev = NULL;
}
pdf_node::~pdf_node()
{
    if (!this->_is_small && this->_data_ptr != NULL)
        free(this->_data_ptr);
}
char* pdf_node::data()
{
    return this->_data_ptr;
}
const char pdf_node::operator[](size_t index) const
{
    if (index >= this->_size)
    {
        throw std::out_of_range("Index out of range");
    }
    return this->_data_ptr[index];
}
size_t pdf_node::size()
{
    return this->_size;
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

// ponytail: replaced linked list with std::vector<std::unique_ptr<pdf_node>>
// push_front pushes to vector back (top of stack)
// pop_front pops from vector back (top of stack) — O(1)
// get(i) returns element at position size()-1-i from top — O(1)
pdf_deque::pdf_deque()
{
}

pdf_deque::~pdf_deque()
{
}

bool pdf_deque::empty()
{
    return _vec.empty();
}

void pdf_deque::clear()
{
    _vec.clear();
}

void pdf_deque::push_back(const void* data, size_t size)
{
    _vec.insert(_vec.begin(), std::unique_ptr<pdf_node>(new pdf_node(data, size)));
}

void pdf_deque::push_front(const void* data, size_t size)
{
    _vec.push_back(std::unique_ptr<pdf_node>(new pdf_node(data, size)));
}

std::unique_ptr<pdf_node> pdf_deque::pop_front()
{
    if (_vec.empty())
        return std::unique_ptr<pdf_node>(new pdf_node("", 0));
    auto node = std::move(_vec.back());
    _vec.pop_back();
    return node;
}

std::unique_ptr<pdf_node> pdf_deque::pop_back()
{
    if (_vec.empty())
        return std::unique_ptr<pdf_node>(new pdf_node("", 0));
    auto node = std::move(_vec.front());
    _vec.erase(_vec.begin());
    return node;
}

pdf_node& pdf_deque::get(int index)
{
    if (index < 0 || (size_t)index >= _vec.size())
        throw std::out_of_range("Index out of range");
    return *_vec[_vec.size() - 1 - index];
}

size_t pdf_deque::size()
{
    return _vec.size();
}
