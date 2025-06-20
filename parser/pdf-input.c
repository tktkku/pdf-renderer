#include "pdf.h"
#include "pdf-private.h"

int pdf_input_file(pdf_input_t** input, const char* filename)
{
    if (input == NULL || filename == NULL) return -1;
    FILE* fp = fopen(filename, "rb");
    if (!fp) return -1;
    *input = (pdf_input_t*)malloc(sizeof(pdf_input_t));
    (*input)->type = PDF_INPUT_TYPE_FILE;
    (*input)->file = fp;
    return 0;
}

int pdf_input_buffer(pdf_input_t** input, const char* data, size_t size)
{
    if (input == NULL || data == NULL || size <= 0) 
        return -1;
    *input = (pdf_input_t*)malloc(sizeof(pdf_input_t));
    (*input)->type = PDF_INPUT_TYPE_BUFFER;
    (*input)->buffer.data = data;
    (*input)->buffer.size = size;
    (*input)->buffer.pos = 0;
    return 0;
}

size_t pdf_input_read(pdf_input_t* input, void* ptr, size_t size)
{
    switch (input->type)
    {
        case PDF_INPUT_TYPE_FILE:
            return fread(ptr, 1, size, input->file);
            break;
        case PDF_INPUT_TYPE_BUFFER:
            size_t remaining = input->buffer.size - input->buffer.pos;
            size_t read_size = (size < remaining) ? size : remaining;
            if (read_size > 0)
            {
                memcpy(ptr, input->buffer.data + input->buffer.pos, read_size);
                input->buffer.pos += read_size;
            }
            return read_size;
            break;
        default:
            return 0;
    }
}

int pdf_input_seek(pdf_input_t* input, long offset, int whence)
{
    switch (input->type)
    {
        case PDF_INPUT_TYPE_FILE:
            return fseek(input->file, offset, whence);
            break;
        case PDF_INPUT_TYPE_BUFFER:
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
        default:
            return -1;
    }
}

long pdf_input_tell(pdf_input_t* input)
{
    switch (input->type)
    {
        case PDF_INPUT_TYPE_FILE:
            return ftell(input->file);
            break;
        case PDF_INPUT_TYPE_BUFFER:
            return input->buffer.pos;
            break;
        default:
            return -1;
            break;
    }
}

void pdf_input_close(pdf_input_t* input)
{
    if (input->type == PDF_INPUT_TYPE_FILE)
    {
        fclose(input->file);
    }
    free(input);
}