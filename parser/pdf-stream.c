#include "pdf.h"
#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
void pdf_stream_close(pdf_stream_t* stream)
{
    inflateEnd(&(stream->decomp.flate));

    if (stream->parser)
    {
        pdf_parser_free(stream->parser);
        stream->parser = NULL;
    }
}

void pdf_stream_free(pdf_stream_t* stream)
{
    if (stream == NULL)
        return;
    if (stream->filter)
    {
        free(stream->filter);
        stream->filter = NULL;
    }

    if (stream->decomp.buf)
    {
        free(stream->decomp.buf);
        stream->decomp.buf = NULL;
    }

    free(stream);
    stream = NULL;
}

pdf_stream_t* pdf_stream_init(pdf_file_t* pdf, pdf_obj_t* obj, int len, int offset)
{
    if (pdf == NULL || obj == NULL)
    {
        return NULL;
    }
    pdf_dict_t* content_dict = obj->value->val.dict;
    const char* filter = pdf_dict_get_name(content_dict, "/Filter");
    pdf_array_t* filter_arr = NULL;

    if (filter == NULL)
    {
        filter_arr = pdf_dict_get_array(content_dict, "/Filter");

        if (filter_arr != NULL && filter_arr->num_elements == 1)
        {
            filter = filter_arr->values[0]->val.name;
        }
    }
    pdf_dict_t* parms_dict = pdf_dict_get_dict(content_dict, "/DecodeParms");
    /**
     * 1 no prediction
     * 2 TIFF predictor 2
     * 10 PNG prediction (on encoding, PNG None on all rows)
     * 11 PNG prediction (on encoding, PNG Sub on all rows)
     * 12 PNG prediction (on encoding, PNG Up on all rows)
     * 13 PNG prediction (on encoding, PNG Average on all rows)
     * 14 PNG prediction (on encoding, PNG Paeth on all rows)
     * 15 PNG prediction (on encoding, PNG optimum)
     */
    int predictor = 1,
        colors = 1, bitspercomponent = 8, columns = 1, earlychange = 1;
    if (parms_dict != NULL)
    {
        predictor = pdf_dict_get_number(parms_dict, "/Predictor");
        if (predictor < 1) predictor = 1;

        colors = pdf_dict_get_number(parms_dict, "/Colors");
        if (colors < 1 || colors > 4) colors = 1;

        bitspercomponent = pdf_dict_get_number(parms_dict, "/BitsPerComponent");
        if (bitspercomponent != 1
            && bitspercomponent != 4
            && bitspercomponent != 8
            && bitspercomponent != 16) bitspercomponent = 8;

        columns = pdf_dict_get_number(parms_dict, "/Columns");
        if (columns < 1) columns = 1;

        earlychange = pdf_dict_get_number(parms_dict, "/EarlyChange");
        if (earlychange != 0 && earlychange != 1) earlychange = 1;
    }

    pdf_stream_t* s = (pdf_stream_t*)calloc(1, sizeof(pdf_stream_t));
    s->pdf = pdf;
    s->obj = obj;
    s->stream_len = len;
    s->stream_offset = offset;
    s->processed = 0;
    s->readin_len = 0;
    s->predictor = predictor;
    s->colors = colors;
    s->bitspercomponent = bitspercomponent;
    s->columns = columns;
    s->earlychange = earlychange;
    if (filter != NULL)
    {
        s->filter = (struct pdf_value*)calloc(1, sizeof(struct pdf_value));
        s->filter->type = NAME;
        s->filter->val.name = (char*)filter;
        s->filter->value_len = strlen(filter);
    }
    else if (filter_arr != NULL)
    {
        s->filter = (struct pdf_value*)calloc(1, sizeof(struct pdf_value));
        s->filter->type = ARRAY;
        s->filter->val.array = filter_arr;
    }

    return s;
}
void pdf_stream_open(pdf_stream_t* stream)
{
    if (stream == NULL || stream->pdf == NULL)
        return;
    stream->parser = pdf_parser_init(stream->pdf, STREAM_READER, stream);
    stream->processed = 0;
    stream->readin_len = 0;
    stream->decomp.flate.avail_in = 0;
    stream->decomp.flate.next_in = NULL;
    stream->decomp.flate.zalloc = NULL;
    stream->decomp.flate.zfree = NULL;
    stream->decomp.flate.opaque = NULL;
    inflateInit(&(stream->decomp.flate));

    stream->decomp.buf = (unsigned char*)malloc(4096);
    stream->decomp.buf_size = 4096;
    stream->decomp.cur_pos = 0;
    stream->decomp.len = 0;
    fseek(stream->pdf->pFile, stream->stream_offset, SEEK_SET);
}
/**
 * -1: error
 * 0: complete
 * > 0: bytes
 */
int pdf_stream_get_data(pdf_stream_t* stream, unsigned char* buf, int size)
{
    if (stream == NULL || buf == NULL || size <= 0)
        return -1;
    int ret = 0;
    if (stream->filter != NULL)
    {
        if (stream->filter->type == NAME)
        {
            if (strcmp(stream->filter->val.name, "/FlateDecode") == 0)
            {
                if (stream->decomp.cur_pos >= stream->decomp.len && stream->readin_len < stream->stream_len)
                {
                    memset(stream->decomp.buf, 0, stream->decomp.buf_size);
                    int read_size = stream->stream_len - stream->processed;
                    read_size = MIN(stream->decomp.buf_size, read_size);
                    fseek(stream->pdf->pFile, stream->stream_offset + stream->readin_len, SEEK_SET);
                    stream->decomp.len = stream->decomp.flate.avail_in
                        = fread(stream->decomp.buf, 1, read_size, stream->pdf->pFile);
                    if (stream->decomp.flate.avail_in == 0)
                    {
                        return 0;
                    }
                    stream->decomp.flate.next_in = stream->decomp.buf;
                    stream->decomp.cur_pos = 0;
                    stream->readin_len += stream->decomp.len;
                }
                if (stream->processed < stream->stream_len)
                {
                    stream->decomp.flate.avail_out = size;
                    stream->decomp.flate.next_out = buf;
                    int zret = inflate(&(stream->decomp.flate), Z_NO_FLUSH);
                    if (zret == Z_STREAM_ERROR || zret == Z_DATA_ERROR || zret == Z_MEM_ERROR || zret == Z_BUF_ERROR)
                    {
                        return 0;
                    }
                    stream->decomp.cur_pos += (stream->decomp.flate.total_in - stream->processed);
                    stream->processed = stream->decomp.flate.total_in;
                    ret = size - stream->decomp.flate.avail_out;
                }
            }
        }
    }
    else
    {
        ret = fread(buf, 1, size, stream->pdf->pFile);
        if (ret < 0)
        {
            return -1;
        }
        stream->readin_len += ret;
    }

    return ret;
}
static void _png_sub(unsigned char* start, int columns, int colors)
{
    for (int k = 0; k < colors; k++)
    {
        *(start + k) += 0;
    }
    for (int j = 1; j < columns; j++)
    {
        int jcolors = j * colors;
        for (int k = 0; k < colors; k++)
        {
            *(start + jcolors + k) += *(start + (jcolors - colors) + k);
        }
        
    }
}
static void _png_up(unsigned char* start, unsigned char* up, int columns, int colors)
{
    if (start == NULL || up == NULL) return;
    for (int j = 0; j < columns; j++)
    {
        int jcolors = j * colors;
        for (int k = 0; k < colors; k++)
        {
            *(start + jcolors + k) += *(up + jcolors + k);
        }
    }
}

static void _png_average(unsigned char* start, unsigned char* up, int columns, int colors)
{
    if (start == NULL) return;
    if (up != NULL)
    {
        for (int k = 0; k < colors; k++)
        {
            *(start + k) += (*(up + k) + 0) / 2;
        }
        for (int j = 1; j < columns; j++)
        {
            int jcolors = j * colors;
            for (int k = 0; k < colors; k++)
            {
                *(start + jcolors + k) += (*(up + jcolors + k) + *(start + (jcolors - colors) + k)) / 2;
            }
        }
    }
    else
    {
        for (int j = 1; j < columns; j++)
        {
            int jcolors = j * colors;
            for (int k = 0; k < colors; k++)
            {
                *(start + jcolors + k) += (0 + *(start + (jcolors - colors) + k)) / 2;
            }
        }
    }
}

static void _png_paeth(unsigned char* start, unsigned char* up, int columns, int colors)
{
    if (start == NULL) return;
    if (up != NULL)
    {
        for (int k = 0; k < colors; k++)
        {
            unsigned char ra = 0;
            unsigned char rb = *(up + k);
            unsigned char rc = 0;
            short p = ra + (rb - rc);
            unsigned char pa = abs(p - ra);
            unsigned char pb = abs(p - rb);
            unsigned char pc = abs(p - rc);
            unsigned char paeth = *(start + k);
            if (pa <= pb && pa <= pc)
            {
                paeth += ra;
            }
            else if (pb <= pc)
            {
                paeth += rb;
            }
            else
            {
                paeth += rc;
            }
            *(start + k) = paeth;
        }
        for (int j = 1; j < columns; j++)
        {
            int jcolors = j * colors;
            for (int k = 0; k < colors; k++)
            {
                unsigned char ra = *(start + (jcolors - colors) + k);
                unsigned char rb = *(up + jcolors + k);
                unsigned char rc = *(up + (jcolors - colors) + k);
                short p = ra + (rb - rc);
                unsigned char pa = abs(p - ra);
                unsigned char pb = abs(p - rb);
                unsigned char pc = abs(p - rc);

                unsigned char paeth = *(start + jcolors + k);
                if (pa <= pb && pa <= pc)
                {
                    paeth += ra;
                }
                else if (pb <= pc)
                {
                    paeth += rb;
                }
                else
                {
                    paeth += rc;
                }
                *(start + jcolors + k) = paeth;
            }
        }
    }
    else
    {
        for (int j = 1; j < columns; j++)
        {
            int jcolors = j * colors;
            for (int k = 0; k < colors; k++)
            {
                unsigned char ra = *(start + (jcolors - colors) + k);
                unsigned char rb = 0;
                unsigned char rc = 0;
                short p = ra + (rb - rc);
                unsigned char pa = abs(p - ra);
                unsigned char pb = abs(p - rb);
                unsigned char pc = abs(p - rc);

                unsigned char paeth = *(start + jcolors + k);
                if (pa <= pb && pa <= pc)
                {
                    paeth += ra;
                }
                else if (pb <= pc)
                {
                    paeth += rb;
                }
                else
                {
                    paeth += rc;
                }
                *(start + jcolors + k) = paeth;
            }
        }
    }
}
void pdf_stream_get_all(pdf_stream_t* stream, unsigned char** buffer, int* size)
{
    if (stream == NULL || buffer == NULL || *buffer == NULL) return;
    unsigned char* start = NULL;
    int ret = 0, off = 0;
    unsigned char tmp[4096];
    pdf_stream_open(stream);
    while ((ret = pdf_stream_get_data(stream, tmp, sizeof(tmp))) > 0)
    {
        if (start == NULL)
        {
            start = (unsigned char*)malloc(ret);
        }
        else
        {
            unsigned char* p = realloc(start, off + ret);
            if (p == NULL)
            {
                free(start);
                return;
            }
            start = p;
        }
        memcpy(start + off, tmp, ret);
        off += ret;
    }
    if (stream->predictor >= 10 && stream->predictor != 15)
    {
        int stride = stream->columns * stream->colors * stream->bitspercomponent / 8;
        int rows = off / (stride + 1);
        unsigned char* data1 = (unsigned char*)malloc(off);
        memcpy(data1, start, off);
        for (int i = 0; i < rows; i++)
        {
            int istride = i * stride;
            memcpy(start + istride, data1 + istride + i + 1, stride);
            unsigned char f = *(data1 + istride + i);
            switch (f)
            {
            case 1:
                _png_sub(start + istride, stream->columns, stream->colors);
                break;
            case 2:
                _png_up(start + istride, start + istride - stride, stream->columns, stream->colors);
                break;
            case 3:
                _png_average(start + istride, start + istride - stride, stream->columns, stream->colors);
                break;
            case 4:
                _png_paeth(start + istride, start + istride - stride, stream->columns, stream->colors);
            default:
                break;
            }
        }
        off = stride * rows;
        free(data1);
    }
    pdf_stream_close(stream);

    *buffer = start;
    *size = off;
}