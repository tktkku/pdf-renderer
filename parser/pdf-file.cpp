#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "zlib.h"

int _read_line(pdf_file_t* pdf, char* buf, int size)
{
    if (pdf == NULL || pdf->pFile == NULL)
        return false;
    memset(buf, 0, size);
    size_t ret = 0;
    char c = '\0';
    int cnt = 0;
    while ((ret = fread(&c, 1, 1, pdf->pFile)) > 0 && cnt < size)
    {
        if (c == '\n' || c == '\r')
        {
            while ((ret = fread(&c, 1, 1, pdf->pFile)) > 0)
            {
                if (c == '\n' || c == '\r')
                {
                    cnt++;
                }
                else
                {
                    fseek(pdf->pFile, -1, SEEK_CUR);
                    break;
                }
            }

            break;
        }
        else
        {
            buf[cnt] = c;
        }
        cnt++;
    }

    return cnt;
}
bool _check_version(pdf_file_t* pdf)
{
    char buffer[1024] = { 0 };
    fseek(pdf->pFile, 0, SEEK_SET);
    _read_line(pdf, buffer, sizeof(buffer));
    if (memcmp(buffer, "%PDF-", 5) != 0)
    {
        return false;
    }

    return true;
}

bool _read_xref_table(pdf_file_t* pdf)
{
    char buffer[1024] = { 0 };
    int start_index = 0, num = 0;

    _read_line(pdf, buffer, sizeof(buffer)); // n n
    char* token = strtok(buffer, " ");
    if (token == NULL) return false;
    start_index = atoi(token);
    token = strtok(NULL, " ");
    if (token == NULL) return false;
    num = atoi(token);

    int seq = start_index;
    for (int i = 0; i < num; i++, seq++)
    {
        xref_t* xrefs = (xref_t*)malloc(sizeof(xref_t));
        xrefs->sequence = seq;

        _read_line(pdf, buffer, sizeof(buffer));
        token = strtok(buffer, " ");
        xrefs->type = UNCOMPRESSED;
        xrefs->uncompressed.offset = strtol(token, NULL, 10);
        token = strtok(NULL, " ");
        xrefs->generation = atoi(token);
        token = strtok(NULL, " ");
        xrefs->inuse = *token;
        pdf->xref_table.push_back(xrefs);
    }
    return true;
}

bool _read_xref_and_trailer(pdf_file_t* pdf, int *root_obj_ref, int *info_obj_ref)
{
    if (pdf == NULL || pdf->pFile == NULL || pdf->data_len == 0)
    {
        return false;
    }

    char buffer[1024] = { 0 };
    fseek(pdf->pFile, pdf->data_len - 128, SEEK_SET);
    int ret = 0;
    while ((ret = _read_line(pdf, buffer, sizeof(buffer))) > 0)
    {
        if (strcmp(buffer, "startxref") == 0)
        {
            break;
        }
    }
    ret = _read_line(pdf, buffer, sizeof(buffer));
    long xref_offset = strtol(buffer, NULL, 10);
    fseek(pdf->pFile, xref_offset, SEEK_SET);
    ret = _read_line(pdf, buffer, sizeof(buffer));

    if (strcmp(buffer, "xref") == 0)
    {
        while (true)
        {
            ret = _read_line(pdf, buffer, sizeof(buffer)); // n n
            if (strcmp(buffer, "trailer") == 0)
            {
                break;
            }
            fseek(pdf->pFile, -(ret + 1), SEEK_CUR);
            if (!_read_xref_table(pdf))
            {
                return false;
            }
        }
        // read trailer
        PdfParser parser(pdf, FILE_READER, pdf->pFile);
        PdfToken* tk = parser.getNextToken();
        if (tk == NULL || tk->getType() != TOKEN_DICT_BEG)
        {
            parser.freeToken(tk);
            tk = NULL;
            return false;
        }
        parser.freeToken(tk);
        tk = NULL;
        PdfDict* trailer = parser.buildDict();
        if (trailer == NULL)
        {
            return false;
        }
        else
        {
            auto& trailer_ref = *trailer;
            *root_obj_ref = trailer_ref["/Root"].indirect;
            *info_obj_ref = trailer_ref["/Info"].indirect;

            if (trailer_ref["/Prev"].type == NUMBER)
            {
                int pre_offset = trailer_ref["/Prev"].number;
                fseek(pdf->pFile, pre_offset, SEEK_SET);
                _read_line(pdf, buffer, sizeof(buffer));// xref skip this line
                if (!_read_xref_table(pdf))
                {
                    return false;
                }
            }
        }
        delete trailer;
        return true;
    }
    else //if (strstr(buffer, "obj") != NULL)
    {
        PdfParser parser(pdf, FILE_READER, pdf->pFile);
        PdfObj* xref_obj = parser.buildObj();
        if (xref_obj == NULL)
        {
            return false;
        }
        auto& xref_dict = *(xref_obj->value->dict);
        *info_obj_ref = xref_dict["/Info"].indirect;
        *root_obj_ref = xref_dict["/Root"].indirect;

        //xref_ref["/Type"]; // XRef
        int size = xref_dict["/Size"].number;
        PdfArray* index_arr = NULL;
        PdfArray* w_aar = NULL;
        if (xref_dict["/Index"].type != ARRAY)
        {
            index_arr = new PdfArray;

            PdfValue* values = new PdfValue;
            values->type = NUMBER;
            values->number = 0;
            index_arr->push(values);

            PdfValue* values1 = new PdfValue;
            values1->type = NUMBER;
            values1->number = size;
            index_arr->push(values1);

            xref_dict["/Index"].type = ARRAY;
            xref_dict["/Index"].array = index_arr;
        }
        else
        {
            index_arr = xref_dict["/Index"].array;
        }
        if (xref_dict["/W"].type != ARRAY)
        {
            return false;
        }
        else
        {
            w_aar = xref_dict["/W"].array;
        }

        int w0 = (*w_aar)[0]->number;
        int w1 = (*w_aar)[1]->number;
        int w2 = (*w_aar)[2]->number;

        unsigned char* start = NULL;
        int len;
        pdf_stream_get_all(xref_obj->stream, &start, &len);

        for (int i = 0; i < index_arr->size(); i += 2)
        {
            int start_index = (*index_arr)[i]->number;
            int num = (*index_arr)[i + 1]->number;

            int seq = start_index;

            for (int j = 0; j < num; j++)
            {
                xref_t* xref = (xref_t*)malloc(sizeof(xref_t));
                int type = 0;
                for (int i = 0; i < w0; i++)
                {
                    type = type << 8;
                    type = type | *start;
                    start += 1;
                }
                int part2 = 0;
                for (int i = 0; i < w1; i++)
                {
                    part2 = part2 << 8;
                    part2 = part2 | *start;
                    start += 1;
                }
                int part3 = 0;
                for (int i = 0; i < w2; i++)
                {
                    part3 = part3 << 8;
                    part3 = part3 | *start;
                    start += 1;
                }
                xref->sequence = seq;
                if (type == 0) // free objects
                {
                    // object-ref generation
                    xref->type = COMPRESSED;
                    xref->compressed.ref = part2;
                    xref->compressed.index = 0;
                    xref->generation = part3;
                    xref->inuse = 'f';
                }
                else if (type == 1) // not be compressed objects
                {
                    // offset generation
                    xref->type = UNCOMPRESSED;
                    xref->uncompressed.offset = part2;
                    xref->generation = part3;
                    xref->inuse = 'n';
                }
                else if (type == 2) // compressed objects
                {
                    // object-ref index
                    // generation shall be 0
                    xref->type = COMPRESSED;
                    xref->compressed.ref = part2;
                    xref->compressed.index = part3;
                    xref->generation = 0;
                    xref->inuse = 'n';
                }
                seq++;
                pdf->xref_table.push_back(xref);
            }
        }

        delete xref_obj;
        return true;
    }

    return false;
}

pdf_file_t* pdf_file_read_file(const char* file_name)
{
    if (file_name == NULL)
    {
        return NULL;
    }
    FILE* file = fopen(file_name, "rb");
    if (file == NULL)
    {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file); //fseek(file, 0, SEEK_SET);

    pdf_file_t* pdf_file = (pdf_file_t*)malloc(sizeof(pdf_file_t));
    memset(pdf_file, 0, sizeof(pdf_file_t));
    pdf_file->pFile = file;
    pdf_file->data_len = file_size;

    if (!_check_version(pdf_file))
    {
        pdf_file_free(pdf_file);
        return NULL;
    }
    int root_obj_ref = -1, info_obj_ref = -1;
    if (!_read_xref_and_trailer(pdf_file, &root_obj_ref, &info_obj_ref))
    {
        pdf_file_free(pdf_file);
        return NULL;
    }

    PdfObj* root_obj = pdf_file_get_obj(pdf_file, root_obj_ref);
    if (root_obj == NULL)
    {
        pdf_file_free(pdf_file);
        return NULL;
    }
    auto& root_dict = *(root_obj->value->dict);
    if (root_dict["/Names"].type == DICT)
    {
        // /Dests
        // /AP
        // /JavaScript
        // /Pages
        // /Templates
        // /IDS
        // /URLS
        // /EmbeddedFiles
        PdfDict* names_dict = root_dict["/Names"].dict;
        if ((*names_dict)["/EmbeddedFiles"].type == INDIRECT)
        {
            int embedded_ref = (*names_dict)["/EmbeddedFiles"].indirect;
            PdfObj* embedded_obj1 = pdf_file_get_obj(pdf_file, embedded_ref);
            if ((*embedded_obj1->value->dict)["/Names"].type == ARRAY)
            {
                PdfArray* names_aar = (*embedded_obj1->value->dict)["/Names"].array;
                for (int i = 0; i < names_aar->size(); i++)
                {
                    if ((*names_aar)[i]->type == INDIRECT)
                    {
                        PdfObj* embedded_obj2 = pdf_file_get_obj(pdf_file, (*names_aar)[i]->indirect);
                        PdfDict* ef_dict = (*embedded_obj2->value->dict)["/EF"].dict;
                        int ref = (*ef_dict)["/UF"].indirect;
                        PdfObj* embedded_obj = pdf_file_get_obj(pdf_file, ref);
                        unsigned char* embedded_file = NULL;
                        int embedded_file_len = 0;
                        pdf_stream_get_all(embedded_obj->stream, &embedded_file, &embedded_file_len);
                        embedded_file_len += 1;
                    }
                }

            }
        }
        // /AlternatePresentations
        // /Renditions
    }
    PdfDict* acroform_dict = NULL;
    if (root_dict["/AcroForm"].type == DICT)
    {
        acroform_dict = root_dict["/AcroForm"].dict;
    }

    int ref = root_dict["/Pages"].indirect;
    PdfObj* pages_obj = pdf_file_get_obj(pdf_file, ref);
    if (pages_obj == NULL)
    {
        pdf_file_free(pdf_file);
        return NULL;
    }
    auto& pages_dict = *(pages_obj->value->dict);
    int num_pages = pages_dict["/Count"].number;
    PdfArray* kids_arr = pages_dict["/Kids"].array;
    for (int i = 0; i < num_pages; i++)
    {
        ref = (*kids_arr)[i]->indirect;
        PdfObj* page_obj = pdf_file_get_obj(pdf_file, ref);
        if (page_obj == NULL)
        {
            pdf_file_free(pdf_file);
            return NULL;
        }
        pdf_file->pages.push_back(page_obj);
    }

    return pdf_file;
}
int pdf_file_get_pages(pdf_file_t* pdf)
{
    if (pdf == NULL) return 0;
    return pdf->pages.size();
}
PdfPage* pdf_file_get_page(pdf_file_t* pdf, int pageNo)
{
    if (pdf == NULL)
    {
        return NULL;
    }

    PdfObj* page_obj = pdf->pages[pageNo];
    auto& page_obj_dict = *(page_obj->value->dict);
    const char* type = page_obj_dict["/Type"].name;
    if (strcmp(type, "/Page") != 0)
    {
        return NULL;
    }

    int rotate = page_obj_dict["/Rotate"].number;

    int res_ref = -1;
    if (page_obj_dict["/Resources"].type == NUL)
    {
        return NULL;
    }
    else if (page_obj_dict["/Resources"].type == INDIRECT)
    {
        res_ref = page_obj_dict["/Resources"].indirect;
    }
    PdfDict* tmp_dict = NULL;
    if (res_ref == -1)
    {
        PdfDict* res_dict = page_obj_dict["/Resources"].dict;
        tmp_dict = res_dict;
    }
    else
    {
        PdfObj* res_obj = pdf_file_get_obj(pdf, res_ref);
        if (res_obj == NULL)
            return NULL;
        tmp_dict = res_obj->value->dict;
    }
    PdfPage* page = new PdfPage;
    if (page_obj_dict["/Annots"].type == ARRAY)
    {
        page->annots = page_obj_dict["/Annots"].array;
    }
    page->pageNo = pageNo;
    if (tmp_dict != NULL)
    {
        auto& tmp_dict_ref = *tmp_dict;
        page->resources = (pdf_resources_t*)malloc(sizeof(pdf_resources_t));
        page->resources->ext_gstate = tmp_dict_ref["/ExtGState"].dict;
        if (page->resources->ext_gstate == NULL)
        {
            if (tmp_dict_ref["/ExtGState"].type == INDIRECT)
            {
                int ext_ref = tmp_dict_ref["/ExtGState"].indirect;
                PdfObj* ext_obj = pdf_file_get_obj(pdf, ext_ref);
                if (ext_obj != NULL)
                {
                    page->resources->ext_gstate = ext_obj->value->dict;
                }
            }
        }

        if (tmp_dict_ref["/Font"].type == DICT)
        {
            page->resources->font_dict = tmp_dict_ref["/Font"].dict;
        }
        else if (tmp_dict_ref["/Font"].type == INDIRECT)
        {
            int font_ref = tmp_dict_ref["/Font"].indirect;
            PdfObj* font_obj = pdf_file_get_obj(pdf, font_ref);
            page->resources->font_dict = font_obj->value->dict;
        }
        if (tmp_dict_ref["/XObject"].type == DICT)
        {
            page->resources->xobject_dict = tmp_dict_ref["/XObject"].dict;
        }
        else if (tmp_dict_ref["/XObject"].type == INDIRECT)
        {
            int xobj_ref = tmp_dict_ref["/XObject"].indirect;
            PdfObj* xobj_obj = pdf_file_get_obj(pdf, xobj_ref);
            page->resources->xobject_dict = xobj_obj->value->dict;
        }
    }
    int contents_ref = -1;
    if (page_obj_dict["/Contents"].type == NUL)
    {
        return NULL;
    }
    else if (page_obj_dict["/Contents"].type == INDIRECT)
    {
        contents_ref = page_obj_dict["/Contents"].indirect;
        PdfObj* content_obj = pdf_file_get_obj(pdf, contents_ref);
        if (content_obj == NULL)
        {
            delete page;
            return NULL;
        }
        page->contents.push_back(content_obj);
    }
    else if (page_obj_dict["/Contents"].type == ARRAY)
    {
        PdfArray* contents_arr = page_obj_dict["/Contents"].array;
        if (contents_arr == NULL)
        {
            return NULL;
        }
        for (int i = 0; i < contents_arr->size(); i++)
        {
            contents_ref = (*contents_arr)[i]->indirect;
            PdfObj* content_obj = pdf_file_get_obj(pdf, contents_ref);
            if (content_obj == NULL)
            {
                delete page;
                return NULL;
            }
            page->contents.push_back(content_obj);
        }
    }

    page->rotate = rotate;
    if (page_obj_dict["/CropBox"].type == ARRAY)
    {   
        PdfArray* crop_arr = page_obj_dict["/CropBox"].array;
        page->crop_box.x = (*crop_arr)[0]->number;
        page->crop_box.y = (*crop_arr)[1]->number;
        page->crop_box.width = (*crop_arr)[2]->number;
        page->crop_box.height = (*crop_arr)[3]->number;
    }
    if (page_obj_dict["/MediaBox"].type == ARRAY)
    {
        PdfArray* media_arr = page_obj_dict["/MediaBox"].array;
        page->media_box.x = (*media_arr)[0]->number;
        page->media_box.y = (*media_arr)[1]->number;
        page->media_box.width = (*media_arr)[2]->number;
        page->media_box.height = (*media_arr)[3]->number;
    }

    page->pdf = pdf;

    return page;
}

void pdf_file_free_page(pdf_file_t* pdf, PdfPage* page)
{
    if (page == nullptr) return;
    delete page;
}

bool _add_to_obj_table(pdf_file_t* pdf, PdfObj* obj)
{
    if (pdf == NULL || obj == NULL)
        return false;
    pdf->read_objs.push_back(obj);
    return true;
}
PdfObj* _get_obj_from_table(pdf_file_t* pdf, int ref)
{
    if (pdf == NULL || ref < 0)
        return NULL;

    if (pdf->read_objs.size() == 0)
        return NULL;

    if (pdf->read_objs.size() > 0)
    {
        for (int i = 0; i < pdf->read_objs.size(); i++)
        {
            if (pdf->read_objs[i]->seq == ref)
            {
                pdf->read_objs[i]->pdf = pdf;
                return pdf->read_objs[i];
            }
        }
    }

    return NULL;
}

PdfObj* pdf_file_get_obj(pdf_file_t* pdf, int ref)
{
    if (pdf == NULL || ref < 0)
    {
        return NULL;
    }

    PdfObj* ret_obj = _get_obj_from_table(pdf, ref);
    if (ret_obj != NULL)
        return ret_obj;

    if (pdf->xref_table.size() == 0)
    {
        return NULL;
    }
    int offset = -1;
    for (int i = 0; i < pdf->xref_table.size(); i++)
    {
        if (pdf->xref_table[i]->sequence == ref)
        {
            if (pdf->xref_table[i]->type == UNCOMPRESSED)
            {
                offset = pdf->xref_table[i]->uncompressed.offset;
                if (offset == -1)
                {
                    return NULL;
                }
                fseek(pdf->pFile, offset, SEEK_SET);
                PdfParser parser(pdf, FILE_READER, pdf->pFile);
                PdfToken* tk = parser.getNextToken();
                if (tk == NULL || tk->getType() != TOKEN_OBJ_BEG)
                {
                    parser.freeToken(tk);
                    return NULL;
                }
                PdfObj* obj = parser.buildObj();
                if (obj == NULL)
                {
                    parser.freeToken(tk);
                    return NULL;
                }
                obj->seq = ref;
                obj->pdf = pdf;
                _add_to_obj_table(pdf, obj);
                parser.freeToken(tk);
                return obj;
            }
            else
            {
                int obj_ref = pdf->xref_table[i]->compressed.ref;
                PdfObj* objs_obj = pdf_file_get_obj(pdf, obj_ref);
                if (objs_obj == NULL)
                {
                    return NULL;
                }
                auto& objs_dict = *(objs_obj->value->dict);
                //objs_dict["/Type"]; // ObjStm
                int num_pairs = objs_dict["/N"].number;
                int first_offset = objs_dict["/First"].number;

                unsigned char* start = NULL;
                int size;
                pdf_stream_get_all(objs_obj->stream, &start, &size);
                pdf_buffer_t b1;
                b1.buffer = start;
                b1.buffer_size = size;
                b1.processed = 0;
                PdfParser parser(pdf, BUFFER_READER, &b1);
                PdfToken* tk = NULL;
                for (int j = 0; j < num_pairs; j++)
                {
                    tk = parser.getNextToken();
                    int seq = atoi(tk->getValue());
                    parser.freeToken(tk);
                    tk = NULL;

                    tk = parser.getNextToken();
                    int offset = atoi(tk->getValue());
                    parser.freeToken(tk);
                    tk = NULL;

                    unsigned char* p1 = start + first_offset + offset;
                    pdf_buffer_t b2;
                    b2.buffer = p1;
                    b2.buffer_size = size - (first_offset + offset);
                    b2.processed = 0;
                    PdfParser val_parser(pdf, BUFFER_READER, &b2);

                    PdfToken* tk1 = val_parser.getNextToken();
                    if (tk1 == NULL)
                    {
                        return NULL;
                    }
                    PdfObj* obj = new PdfObj;
                    obj->seq = seq;
                    obj->value = new PdfValue;
                    if (tk1->getType() == TOKEN_DICT_BEG)
                    {
                        PdfDict* obj_dict = val_parser.buildDict();
                        if (obj_dict == NULL)
                        {
                            val_parser.freeToken(tk1);
                            return NULL;
                        }

                        obj->value->type = DICT;
                        obj->value->dict = obj_dict;
                    }
                    else if (tk1->getType() == TOKEN_ARRAY_BEG)
                    {
                        PdfArray* array = val_parser.buildArray();
                        if (array == NULL)
                        {
                            val_parser.freeToken(tk1);
                            return NULL;
                        }
                        obj->value->type = ARRAY;
                        obj->value->array = array;
                    }
                    else
                    {
                        val_parser.freeToken(tk1);
                        return NULL;
                    }
                    val_parser.freeToken(tk1);

                    _add_to_obj_table(pdf, obj);
                }

                return _get_obj_from_table(pdf, ref);
            }
        }
    }
    return NULL;
}
pdf_cmap_t* pdf_file_get_cmap(pdf_file_t* pdf, char* name)
{
    if (pdf == NULL || name == NULL)
    {
        return NULL;
    }
    if (pdf->cmaps.size() > 0)
    {
        for (auto* ptr : pdf->cmaps)
        {
            if (!strcmp(ptr->name, name))
            {
                return ptr;
            }
        }
    }
    FILE* f = NULL;
    char filename[256] = { 0 };
    sprintf(filename, "./CMaps%s", name);
    f = fopen(filename, "r");
    if (f == NULL)
        return NULL;

    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    rewind(f);
    unsigned char* filedata = (unsigned char*)malloc(filesize);
    if (filedata == NULL)
    {
        fclose(f);
        return NULL;
    }
    fread(filedata, filesize, 1, f);
    fclose(f);
    pdf_buffer_t b1;
    b1.buffer = filedata;
    b1.buffer_size = filesize;
    b1.processed = 0;
    PdfParser parser(pdf, BUFFER_READER, &b1);
    pdf_cmap_t* cmap = parser.buildCMap();
    free(filedata);
    cmap->worldwide = true;
    strcpy(cmap->name, name);

    pdf->cmaps.push_back(cmap);
    return cmap;
}

void pdf_file_free(pdf_file_t* file)
{
    if (file == NULL) return;

    if (file->xref_table.size() > 0)
    {
        for (auto* ptr : file->xref_table)
        {
            free(ptr);
        }

    }

    // if (file->pages.size() > 0)
    // {
    //     for (auto* ptr : file->pages)
    //     {
    //         pdf_obj_free(ptr);
    //     }

    // }

    if (file->read_objs.size() > 0)
    {
        for (auto* ptr : file->read_objs)
        {
            delete ptr;
        }
    }
    if (file->cmaps.size() > 0)
    {
        for (auto* ptr : file->cmaps)
        {
            pdf_cmap_free(ptr);
        }
        
    }
    if (file->pFile)
    {
        fclose(file->pFile);
        file->pFile = NULL;
    }
    free(file);
    file = NULL;
}