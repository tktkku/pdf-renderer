#include "pdf.h"
#include "pdf-private.h"
#include "rc4.h"
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
    if (stream->input)
    {
        input_close(stream->input);
        stream->input = NULL;
    }
}

void pdf_stream_free(pdf_stream_t* stream)
{
    if (stream == NULL)
        return;
    if (stream->filter)
    {
        delete stream->filter;
        stream->filter = NULL;
    }

    delete stream;
}

pdf_stream_t* pdf_stream_init(pdf_file_t* pdf, pdf_obj_t* obj, int len, int offset)
{
    if (pdf == NULL || obj == NULL)
    {
        return NULL;
    }
    pdf_dict* content_dict = obj->value->val.dict;
    const char* filter = NULL;
    pdf_array* filter_arr = NULL;
    if (content_dict->has("/Filter"))
    {
        if (content_dict->is_name("/Filter"))
        {
            filter = content_dict->get_name("/Filter");
        }
        else if (content_dict->is_array("/Filter"))
        {
            filter_arr = content_dict->get_array("/Filter");
            if (filter_arr != NULL && filter_arr->size() == 1)
            {
                filter = filter_arr->get(0)->val.name;
            }
        }
    }

    pdf_dict* parms_dict = content_dict->get_dict("/DecodeParms");
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
        predictor = parms_dict->get_number("/Predictor");
        if (predictor < 1) predictor = 1;

        colors = parms_dict->get_number("/Colors");
        if (colors < 1 || colors > 4) colors = 1;

        bitspercomponent = parms_dict->get_number("/BitsPerComponent");
        if (bitspercomponent != 1
            && bitspercomponent != 4
            && bitspercomponent != 8
            && bitspercomponent != 16) bitspercomponent = 8;

        columns = parms_dict->get_number("/Columns");
        if (columns < 1) columns = 1;

        earlychange = parms_dict->get_number("/EarlyChange");
        if (earlychange != 0 && earlychange != 1) earlychange = 1;
    }

    pdf_stream_t* s = new pdf_stream_t{};
    s->pdf = pdf;
    s->obj = obj;
    s->stream_len = len;
    s->stream_offset = offset;
    s->predictor = predictor;
    s->colors = colors;
    s->bitspercomponent = bitspercomponent;
    s->columns = columns;
    s->earlychange = earlychange;
    if (filter != NULL)
    {
        s->filter = new pdf_value_t{};
        s->filter->type = PDF_VALUE_NAME;
        s->filter->val.name = (char*)filter;
        s->filter->value_len = strlen(filter);
    }
    else if (filter_arr != NULL)
    {
        s->filter = new pdf_value_t{};
        s->filter->type = PDF_VALUE_ARRAY;
        s->filter->val.array = filter_arr;
    }

    return s;
}
bool _try_inflate(const unsigned char* data, size_t len)
{
    z_stream strm = {};
    strm.next_in = (Bytef*)data;
    strm.avail_in = len;
    if (inflateInit(&strm) != Z_OK)
    {
        return false;
    }

    unsigned char out[1024];
    strm.next_out = out;
    strm.avail_out = sizeof(out);

    int ret = inflate(&strm, Z_NO_FLUSH);

    inflateEnd(&strm);

    return (ret == Z_STREAM_END || ret == Z_OK);
}
bool _try_inflate_raw(const unsigned char* data, size_t len)
{
    z_stream strm = {};
    strm.next_in = (Bytef*)data;
    strm.avail_in = len;
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK)
    {
        return false;
    }

    unsigned char out[1024];
    strm.next_out = out;
    strm.avail_out = sizeof(out);

    int ret = inflate(&strm, Z_NO_FLUSH);

    inflateEnd(&strm);

    return (ret == Z_STREAM_END || ret == Z_OK);
}

void pdf_stream_open(pdf_stream_t* stream)
{
    if (stream == NULL || stream->pdf == NULL)
        return;
    input_t* input = NULL;
    input_stream(&input, stream);
    stream->input = input;
    stream->parser = pdf_parser_init(stream->pdf, input);
    stream->decomp.flate.avail_in = 0;
    stream->decomp.flate.next_in = NULL;
    stream->decomp.flate.zalloc = NULL;
    stream->decomp.flate.zfree = NULL;
    stream->decomp.flate.opaque = NULL;

    stream->decomp.buf = std::make_unique<unsigned char[]>(stream->stream_len);
    stream->decomp.cur_pos = 0;
    stream->decomp.len = 0;
    input_seek(stream->pdf->input, stream->stream_offset, SEEK_SET);
    stream->decomp.len = input_read(stream->pdf->input, stream->decomp.buf.get(), stream->stream_len);
    if (stream->decomp.len != stream->stream_len)
    {
        printf("pdf_stream_open error, read %d bytes, expect %d bytes\n", stream->decomp.len, stream->stream_len);
        return;
    }
    int off = stream->stream_len - 1;
    while (stream->decomp.buf[off] == '\r' || stream->decomp.buf[off] == '\n')
    {
        stream->decomp.buf[off] = '\0';
        stream->decomp.len--;
        off--;
    }
    if (stream->filter && strcmp(stream->filter->val.name, "/FlateDecode") == 0)
    {
        if (_try_inflate(stream->decomp.buf.get(), stream->decomp.len))
        {
            inflateInit(&stream->decomp.flate);
        }
        else if (_try_inflate_raw(stream->decomp.buf.get(), stream->decomp.len))
        {
            inflateInit2(&stream->decomp.flate, -MAX_WBITS);
        }
        else
        {
            int n = stream->pdf->encrypt_key_len_bits / 8;
            uint8_t* data = new uint8_t[n + 5];
            int off = 0;
            memcpy(data, stream->pdf->encrypt_key, n);
            off += n;
            data[off++] = ((stream->obj->indirect.obj_num >> 0) & 0xFF);
            data[off++] = ((stream->obj->indirect.obj_num >> 8) & 0xFF);
            data[off++] = ((stream->obj->indirect.obj_num >> 16) & 0xFF);
            data[off++] = ((stream->obj->indirect.generation >> 0) & 0xFF);
            data[off++] = ((stream->obj->indirect.generation >> 8) & 0xFF);

            uint8_t hash[16];
            md5(data, off, hash);
            delete[] data;

            rc4_ctx ctx;
            rc4_ks(&ctx, hash, std::min(n + 5, 16));
            rc4_decrypt(&ctx, stream->decomp.buf.get(), stream->decomp.buf.get(), stream->decomp.len);
            if (_try_inflate(stream->decomp.buf.get(), stream->decomp.len))
            {
                inflateInit(&stream->decomp.flate);
            }
            else if (_try_inflate_raw(stream->decomp.buf.get(), stream->decomp.len))
            {
                inflateInit2(&stream->decomp.flate, -MAX_WBITS);
            }
            else
            {
                printf("object %d decompress error\n", stream->obj->indirect.obj_num);
                return;
            }
        }
    }

    stream->decomp.flate.avail_in = stream->decomp.len;
    stream->decomp.flate.next_in = stream->decomp.buf.get();
    stream->decomp.cur_pos = 0;
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
        if (stream->filter->type == PDF_VALUE_NAME)
        {
            if (strcmp(stream->filter->val.name, "/FlateDecode") == 0)
            {
                if (stream->decomp.cur_pos < stream->decomp.len)
                {
                    stream->decomp.flate.avail_out = size;
                    stream->decomp.flate.next_out = buf;
                    int zret = inflate(&(stream->decomp.flate), Z_NO_FLUSH);
                    if (zret == Z_STREAM_ERROR || zret == Z_DATA_ERROR || zret == Z_MEM_ERROR || zret == Z_BUF_ERROR)
                    {
                        printf("inflate data error: %d\n", zret);
                        return 0;
                    }
                    ret = size - stream->decomp.flate.avail_out;
                    stream->decomp.cur_pos = stream->decomp.flate.total_in;
                }
            }
        }
    }
    else
    {
        size_t remaining = stream->decomp.len - stream->decomp.cur_pos;
        size_t read_size = (size < remaining) ? size : remaining;
        if (read_size > 0)
        {
            memcpy(buf, stream->decomp.buf.get() + stream->decomp.cur_pos, read_size);
            stream->decomp.cur_pos += read_size;
        }
        ret = read_size;
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
    if (stream == NULL || buffer == NULL) return;
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
            unsigned char* p = (unsigned char*)realloc(start, off + ret);
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
        memcpy(start, data1 + 1, stride);
        switch (data1[0])
        {
        case 1:
            _png_sub(start, stream->columns, stream->colors);
            break;
        case 2:
            _png_up(start, NULL, stream->columns, stream->colors);
            break;
        case 3:
            _png_average(start, NULL, stream->columns, stream->colors);
            break;
        case 4:
            _png_paeth(start, NULL, stream->columns, stream->colors);
        default:
            break;
        }
        for (int i = 1; i < rows; i++)
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