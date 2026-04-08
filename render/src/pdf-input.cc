#include "pdf.h"
#include "pdf-private.h"

#include <string.h>
int input_file(input_t** input, const char* filename)
{
    if (input == NULL || filename == NULL) return -1;
    FILE* fp = fopen(filename, "rb");
    if (!fp) return -1;
    *input = (input_t*)malloc(sizeof(input_t));
    (*input)->type = INPUT_TYPE_FILE;
    (*input)->file = fp;
    return 0;
}

int input_buffer(input_t** input, const char* data, size_t size)
{
    if (input == NULL || data == NULL || size <= 0) 
        return -1;
    *input = (input_t*)malloc(sizeof(input_t));
    (*input)->type = INPUT_TYPE_BUFFER;
    (*input)->buffer.data = data;
    (*input)->buffer.size = size;
    (*input)->buffer.pos = 0;
    return 0;
}

int input_stream(input_t** input, pdf_stream_t* stream)
{
    if (input == NULL || stream == NULL) 
        return -1;
    *input = (input_t*)malloc(sizeof(input_t));
    (*input)->type = INPUT_TYPE_STREAM;
    (*input)->stream = stream;
    return 0;
}

size_t input_read(input_t* input, void* ptr, size_t size)
{
    if (ptr == NULL || size <= 0) return 0;
    switch (input->type)
    {
        case INPUT_TYPE_FILE:
            return fread(ptr, 1, size, input->file);
            break;
        case INPUT_TYPE_BUFFER:
        {
            size_t remaining = input->buffer.size - input->buffer.pos;
            size_t read_size = (size < remaining) ? size : remaining;
            if (read_size > 0)
            {
                memcpy(ptr, input->buffer.data + input->buffer.pos, read_size);
                input->buffer.pos += read_size;
            }
            return read_size;
        }
        case INPUT_TYPE_STREAM:
        {
            int ret = 0;
            // input->stream->parser->current_pos;
            if (input->stream->decomp.cur_pos < input->stream->decomp.len)
            {
                ret = pdf_stream_get_data(input->stream, (unsigned char*)ptr, size);
            }
            return ret;
        }
        default:
            return 0;
    }
}

int input_seek(input_t* input, long offset, int whence)
{
    switch (input->type)
    {
        case INPUT_TYPE_FILE:
            return fseek(input->file, offset, whence);
            break;
        case INPUT_TYPE_BUFFER:
        {
            size_t new_pos;
            switch (whence)
            {
                case SEEK_SET:
                    new_pos = offset;
                    break;
                case SEEK_CUR:
                    new_pos = input->buffer.pos + offset;
                    break;
                case SEEK_END:
                    new_pos = input->buffer.size + offset;
                    break;
                default:
                    return -1;
            }
            if (new_pos > input->buffer.size) return -1;
            input->buffer.pos = new_pos;
            return 0;
        }  
        default:
            return -1;
    }
}

long input_tell(input_t* input)
{
    switch (input->type)
    {
        case INPUT_TYPE_FILE:
            return ftell(input->file);
            break;
        case INPUT_TYPE_BUFFER:
            return input->buffer.pos;
            break;
        default:
            return -1;
            break;
    }
}

void input_close(input_t* input)
{
    if (input->type == INPUT_TYPE_FILE)
    {
        fclose(input->file);
    }
    free(input);
}