#include "pdf.h"
#include "pdf-private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
void PdfStream::close()
{
    inflateEnd(&(decomp.flate));

    if (parser)
    {
        delete parser;
        parser = NULL;
    }
}
PdfToken* PdfStream::getNextToken()
{
    return parser->getNextToken();
}

PdfStream::~PdfStream()
{
    if (filter)
    {
        free(filter);
        filter = NULL;
    }

    if (decomp.buf)
    {
        free(decomp.buf);
        decomp.buf = NULL;
    }
}

PdfStream::PdfStream(pdf_file_t* pdf, PdfObj* obj, int len, int offset)
{
    auto& content_dict = *obj->value->dict;
    const char* flt = NULL;
    PdfArray* filter_arr = NULL;
    if (content_dict["/Filter"].type == NAME)
    {
        flt = content_dict["/Filter"].name;
    }
    else if (content_dict["/Filter"].type == ARRAY)
    {
        filter_arr = content_dict["/Filter"].array;

        if (filter_arr != NULL && filter_arr->size() == 1)
        {
            flt = (*filter_arr)[0]->name;
        }
    }
    PdfDict* parms_dict = NULL;
    if (content_dict["/DecodeParms"].type == DICT)
    {
        parms_dict = content_dict["/DecodeParms"].dict;
    }
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
        if ((*parms_dict)["/Predictor"].type == NUMBER) 
        {
            predictor = (*parms_dict)["/Predictor"].number;
        }
        if (predictor < 1) predictor = 1;

        if ((*parms_dict)["/Colors"].type == NUMBER)
        {
            colors = (*parms_dict)["/Colors"].number;
        }
        if (colors < 1 || colors > 4) colors = 1;
        
        if ((*parms_dict)["/BitsPerComponent"].type == NUMBER)
        {
            bitspercomponent = (*parms_dict)["/BitsPerComponent"].number;
        }
        if (bitspercomponent != 1
            && bitspercomponent != 4
            && bitspercomponent != 8
            && bitspercomponent != 16) bitspercomponent = 8;

        if ((*parms_dict)["/Columns"].type == NUMBER)
        {
            columns = (*parms_dict)["/Columns"].number;
        }
        if (columns < 1) columns = 1;

        if ((*parms_dict)["/EarlyChange"].type == NUMBER)
        {
            earlychange = (*parms_dict)["/EarlyChange"].number;
        }
        if (earlychange != 0 && earlychange != 1) earlychange = 1;
    }

    this->pdf = pdf;
    this->obj = obj;
    this->stream_len = len;
    this->stream_offset = offset;
    this->processed = 0;
    this->readin_len = 0;
    this->predictor = predictor;
    this->colors = colors;
    this->bitspercomponent = bitspercomponent;
    this->columns = columns;
    this->earlychange = earlychange;
    if (flt != NULL)
    {
        this->filter = new PdfValue;
        this->filter->type = NAME;
        this->filter->name = (char*)flt;
        this->filter->value_len = strlen(flt);
    }
    else if (filter_arr != NULL)
    {
        this->filter = new PdfValue;
        this->filter->type = ARRAY;
        this->filter->array = filter_arr;
    }
}
void PdfStream::open()
{
    if (pdf == NULL)
        return;
    this->parser = new PdfParser(this->pdf, STREAM_READER, this);
    this->decomp.flate.avail_in = 0;
    this->decomp.flate.next_in = NULL;
    this->decomp.flate.zalloc = NULL;
    this->decomp.flate.zfree = NULL;
    this->decomp.flate.opaque = NULL;
    inflateInit(&(this->decomp.flate));

    this->decomp.buf = (unsigned char*)malloc(4096);
    this->decomp.buf_size = 4096;
    this->decomp.cur_pos = 0;
    this->decomp.len = 0;
    fseek(this->pdf->pFile, this->stream_offset, SEEK_SET);
}
/**
 * -1: error
 * 0: complete
 * > 0: bytes
 */
int PdfStream::getData(unsigned char* buf, int size)
{
    if (buf == NULL || size <= 0)
        return -1;
    int ret = 0;
    if (this->filter != NULL)
    {
        if (this->filter->type == NAME)
        {
            if (strcmp(this->filter->name, "/FlateDecode") == 0)
            {
                if (this->decomp.cur_pos >= this->decomp.len && this->readin_len < this->stream_len)
                {
                    memset(this->decomp.buf, 0, this->decomp.buf_size);
                    int read_size = this->stream_len - this->processed;
                    read_size = MIN(this->decomp.buf_size, read_size);
                    fseek(this->pdf->pFile, this->stream_offset + this->readin_len, SEEK_SET);
                    this->decomp.len = this->decomp.flate.avail_in
                        = fread(this->decomp.buf, 1, read_size, this->pdf->pFile);
                    if (this->decomp.flate.avail_in == 0)
                    {
                        return 0;
                    }
                    this->decomp.flate.next_in = this->decomp.buf;
                    this->decomp.cur_pos = 0;
                    this->readin_len += this->decomp.len;
                }
                if (this->processed < this->stream_len)
                {
                    this->decomp.flate.avail_out = size;
                    this->decomp.flate.next_out = buf;
                    int zret = inflate(&(this->decomp.flate), Z_NO_FLUSH);
                    if (zret == Z_STREAM_ERROR || zret == Z_DATA_ERROR || zret == Z_MEM_ERROR || zret == Z_BUF_ERROR)
                    {
                        return 0;
                    }
                    this->decomp.cur_pos += (this->decomp.flate.total_in - this->processed);
                    this->processed = this->decomp.flate.total_in;
                    ret = size - this->decomp.flate.avail_out;
                }
            }
        }
    }
    else
    {
        ret = fread(buf, 1, size, this->pdf->pFile);
        if (ret < 0)
        {
            return -1;
        }
        this->readin_len += ret;
    }

    return ret;
}

void PdfStream::getAll(unsigned char** buffer, int* size)
{
    unsigned char* start = NULL;
    int ret = 0, off = 0;
    unsigned char tmp[4096];
    open();
    while ((ret = getData(tmp, sizeof(tmp))) > 0)
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
    if (this->predictor == 12) // PNG UP
    {
        int stride = this->columns * this->colors * this->bitspercomponent / 8;
        int rows = off / (stride + 1);
        char* data1 = (char*)malloc(off);
        memcpy(data1, start, off);
        /**
         * 2 0 0 0 0 0 255 255
         * 2 2 0 0 0 28 1 1
         * 
         * 0 0 0 0 0 255 255
         * 2 0 0 0 28 1 1 -> 2 0 0 0 28 1 1
         */
        // copy orig to cur
        memcpy(start, data1 + 1, stride);
        // from the second line
        for (int i = 1; i < rows; i++)
        {
            // copy orig to cur
            memcpy(start + i * stride, data1 + i * (stride + 1) + 1, stride);
            char f = *(data1 + i * (stride + 1));
            assert(f == 2);
            for (int j = 0; j < stride; j++)
            {
                // encode: encoded_cur = raw_cur -  encoded_up
                // decode: raw_cur = encoded_cur + decoded_up
                *(start + i * stride + j) += *(start + (i - 1) * stride + j);
            }
        }
        off = stride * rows;
        free(data1);
    }
    close();

    *buffer = start;
    *size = off;
}