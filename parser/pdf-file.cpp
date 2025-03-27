#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>

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

xref_table_t* _read_xref_table(pdf_file_t* pdf)
{
    char buffer[1024] = { 0 };
    int start_index = 0, num = 0;

    _read_line(pdf, buffer, sizeof(buffer)); // n n
    char* token = strtok(buffer, " ");
    if (token == NULL) return NULL;
    start_index = atoi(token);
    token = strtok(NULL, " ");
    if (token == NULL) return NULL;
    num = atoi(token);

    xref_table_t* table = (xref_table_t*)malloc(sizeof(xref_table_t));
    table->size = num;
    table->xrefs = (xref_t*)malloc(sizeof(xref_t) * num);

    int seq = start_index;
    for (int i = 0; i < num; i++, seq++)
    {
        table->xrefs[i].sequence = seq;

        _read_line(pdf, buffer, sizeof(buffer));
        token = strtok(buffer, " ");
        table->xrefs[i].type = UNCOMPRESSED;
        table->xrefs[i].uncompressed.offset = strtol(token, NULL, 10);
        token = strtok(NULL, " ");
        table->xrefs[i].generation = atoi(token);
        token = strtok(NULL, " ");
        table->xrefs[i].inuse = *token;
    }

    return table;
}

bool _read_xref_and_trailer(pdf_file_t* pdf)
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
        pdf->xref_table = NULL;

        while (true)
        {
            ret = _read_line(pdf, buffer, sizeof(buffer)); // n n
            if (strcmp(buffer, "trailer") == 0)
            {
                break;
            }
            fseek(pdf->pFile, -(ret + 1), SEEK_CUR);
            xref_table_t* table = _read_xref_table(pdf);
            if (table == NULL)
            {
                if (pdf->xref_table)
                {
                    free(pdf->xref_table->xrefs);
                    free(pdf->xref_table);
                }
                return false;
            }
            if (pdf->xref_table == NULL)
            {
                pdf->xref_table = table;
            }
            else
            {
                xref_t* t = (xref_t*)realloc(pdf->xref_table->xrefs, (pdf->xref_table->size + table->size) * sizeof(xref_t));
                if (t == NULL)
                {
                    free(table->xrefs);
                    free(table);

                    free(pdf->xref_table->xrefs);
                    free(pdf->xref_table);
                    return false;
                }
                pdf->xref_table->xrefs = t;
                memcpy(pdf->xref_table->xrefs + pdf->xref_table->size, table->xrefs, table->size * sizeof(xref_t));
                pdf->xref_table->size += table->size;
                free(table->xrefs);
                free(table);
            }
        }
        // read trailer
        pdf_parser_t* parser = pdf_parser_init(pdf, FILE_READER, pdf->pFile);
        pdf_parser_token_t* tk = pdf_parser_next_token(parser);
        if (tk == NULL || tk->type != TOKEN_DICT_BEG)
        {
            pdf_parser_token_free(tk);
            tk = NULL;
            pdf_parser_free(parser);
            return false;
        }
        pdf_parser_token_free(tk);
        tk = NULL;
        PdfDict* trailer = pdf_parser_build_dict(parser);
        if (trailer == NULL)
        {
            pdf_parser_free(parser);
            return false;
        }
        else
        {
            pdf->root_obj_ref = pdf_dict_get_ref(trailer, "/Root");
            pdf->info_obj_ref = pdf_dict_get_ref(trailer, "/Info");

            int pre_offset = pdf_dict_get_number(trailer, "/Prev");
            if (pre_offset != -1)
            {
                pdf->current_index = pre_offset;
                fseek(pdf->pFile, pre_offset, SEEK_SET);
                _read_line(pdf, buffer, sizeof(buffer));// xref skip this line
                xref_table_t* table = _read_xref_table(pdf);
                xref_t* t = (xref_t*)realloc(pdf->xref_table->xrefs, (pdf->xref_table->size + table->size) * sizeof(xref_t));
                if (t == NULL)
                {
                    free(table->xrefs);
                    free(table);

                    free(pdf->xref_table->xrefs);
                    free(pdf->xref_table);
                    return false;
                }
                pdf->xref_table->xrefs = t;
                memcpy(pdf->xref_table->xrefs + pdf->xref_table->size, table->xrefs, table->size * sizeof(xref_t));
                pdf->xref_table->size += table->size;
                free(table->xrefs);
                free(table);
            }
        }
        pdf_dict_free(trailer);
        pdf_parser_free(parser);
        return true;
    }
    else //if (strstr(buffer, "obj") != NULL)
    {
        pdf_parser_t* parser = pdf_parser_init(pdf, FILE_READER, pdf->pFile);
        pdf_obj_t* xref_obj = pdf_parser_build_obj(parser);
        pdf_parser_free(parser);
        if (xref_obj == NULL)
        {
            return false;
        }
        PdfDict* xref_dict = xref_obj->value->val.dict;
        pdf->info_obj_ref = pdf_dict_get_ref(xref_dict, "/Info");
        pdf->root_obj_ref = pdf_dict_get_ref(xref_dict, "/Root");

        pdf_dict_get_name(xref_dict, "/Type"); // XRef
        int size = pdf_dict_get_number(xref_dict, "/Size");
        PdfArray* index_arr = pdf_dict_get_array(xref_dict, "/Index");
        PdfArray* w_aar = pdf_dict_get_array(xref_dict, "/W");
        if (index_arr == NULL)
        {
            index_arr = new PdfArray;

            pdf_value* values = (pdf_value*)malloc(sizeof(pdf_value));
            values->type = NUMBER;
            values->val.number = 0;
            index_arr->push_back(values);
            
            pdf_value* values1 = (pdf_value*)malloc(sizeof(pdf_value));
            values1->type = NUMBER;
            values1->val.number = size;
            index_arr->push_back(values1);

            pdf_dict_add_array(xref_dict, "/Index", index_arr);
        }
        if (w_aar == NULL)
        {
            return false;
        }

        int w0 = (*w_aar)[0]->val.number;
        int w1 = (*w_aar)[1]->val.number;
        int w2 = (*w_aar)[2]->val.number;

        unsigned char* start = NULL;
        int len;
        pdf_stream_get_all(xref_obj->stream, &start, &len);
        pdf->xref_table = (xref_table_t*)malloc(sizeof(xref_table_t));
        pdf->xref_table->size = 0;
        pdf->xref_table->xrefs = NULL;

        for (int i = 0; i < index_arr->size(); i += 2)
        {
            int start_index = (*index_arr)[i]->val.number;
            int num = (*index_arr)[i + 1]->val.number;

            int seq = start_index;

            xref_t* xref = (xref_t*)malloc(sizeof(xref_t) * num);
            for (int j = 0; j < num; j++)
            {
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
                xref[j].sequence = seq;
                if (type == 0) // free objects
                {
                    // object-ref generation
                    xref[j].type = COMPRESSED;
                    xref[j].compressed.ref = part2;
                    xref[j].compressed.index = 0;
                    xref[j].generation = part3;
                    xref[j].inuse = 'f';
                }
                else if (type == 1) // not be compressed objects
                {
                    // offset generation
                    xref[j].type = UNCOMPRESSED;
                    xref[j].uncompressed.offset = part2;
                    xref[j].generation = part3;
                    xref[j].inuse = 'n';
                }
                else if (type == 2) // compressed objects
                {
                    // object-ref index
                    // generation shall be 0
                    xref[j].type = COMPRESSED;
                    xref[j].compressed.ref = part2;
                    xref[j].compressed.index = part3;
                    xref[j].generation = 0;
                    xref[j].inuse = 'n';
                }

                seq++;
            }
            if (pdf->xref_table->xrefs == NULL)
            {
                pdf->xref_table->xrefs = xref;
                pdf->xref_table->size = num;
            }
            else
            {
                xref_t* t = (xref_t*)realloc(pdf->xref_table->xrefs, (pdf->xref_table->size + num) * sizeof(xref_t));
                if (t == NULL)
                {
                    pdf_obj_free(xref_obj);
                    return false;
                }
                pdf->xref_table->xrefs = t;
                memcpy(pdf->xref_table->xrefs + pdf->xref_table->size, xref, num * sizeof(xref_t));
                free(xref);
                pdf->xref_table->size += num;
            }
        }

        pdf_obj_free(xref_obj);
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
    pdf_file->current_index = 0;

    if (!_check_version(pdf_file))
    {
        pdf_file_free(pdf_file);
        return NULL;
    }
    if (!_read_xref_and_trailer(pdf_file))
    {
        pdf_file_free(pdf_file);
        return NULL;
    }

    pdf_obj_t* root_obj = pdf_file_get_obj(pdf_file, pdf_file->root_obj_ref);
    if (root_obj == NULL)
    {
        pdf_file_free(pdf_file);
        return NULL;
    }
    PdfDict* names_dict = pdf_dict_get_dict(root_obj->value->val.dict, "/Names");
    if (names_dict != NULL)
    {
        // /Dests
        // /AP
        // /JavaScript
        // /Pages
        // /Templates
        // /IDS
        // /URLS
        // /EmbeddedFiles
        int embedded_ref = pdf_dict_get_ref(names_dict, "/EmbeddedFiles");
        if (embedded_ref != -1)
        {
            pdf_obj_t* embedded_obj1 = pdf_file_get_obj(pdf_file, embedded_ref);
            PdfArray* names_aar = pdf_dict_get_array(embedded_obj1->value->val.dict, "/Names");
            if (names_aar != NULL)
            {
                for (int i = 0; i < names_aar->size(); i++)
                {
                    if ((*names_aar)[i]->type == INDIRECT)
                    {
                        pdf_obj_t* embedded_obj2 = pdf_file_get_obj(pdf_file, (*names_aar)[i]->val.indirect);
                        PdfDict* ef_dict = pdf_dict_get_dict(embedded_obj2->value->val.dict, "/EF");
                        int ref = pdf_dict_get_ref(ef_dict, "/UF");
                        pdf_obj_t* embedded_obj = pdf_file_get_obj(pdf_file, ref);
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
    PdfDict* acroform_dict = pdf_dict_get_dict(root_obj->value->val.dict, "/AcroForm");
    if (acroform_dict != NULL)
    {

    }

    int ref = pdf_dict_get_ref(root_obj->value->val.dict, "/Pages");
    pdf_obj_t* pages_obj = pdf_file_get_obj(pdf_file, ref);
    if (pages_obj == NULL)
    {
        pdf_file_free(pdf_file);
        return NULL;
    }

    pdf_file->num_pages = pdf_dict_get_number(pages_obj->value->val.dict, "/Count");
    pdf_file->pages = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*) * pdf_file->num_pages);
    PdfArray* kids_arr = pdf_dict_get_array(pages_obj->value->val.dict, "/Kids");
    for (int i = 0; i < pdf_file->num_pages; i++)
    {
        ref = (*kids_arr)[i]->val.indirect;
        pdf_obj_t* page_obj = pdf_file_get_obj(pdf_file, ref);
        if (page_obj == NULL)
        {
            pdf_file_free(pdf_file);
            return NULL;
        }
        pdf_file->pages[i] = page_obj;
    }

    return pdf_file;
}
int pdf_file_get_pages(pdf_file_t* pdf)
{
    if (pdf == NULL) return 0;
    return pdf->num_pages;
}
pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo)
{
    if (pdf == NULL)
    {
        return NULL;
    }

    pdf_obj_t* page_obj = pdf->pages[pageNo];
    PdfDict* page_obj_dict = page_obj->value->val.dict;
    const char* type = pdf_dict_get_name(page_obj_dict, "/Type");
    if (strcmp(type, "/Page") != 0)
    {
        return NULL;
    }

    int rotate = pdf_dict_get_number(page_obj_dict, "/Rotate");

    int contents_ref = pdf_dict_get_ref(page_obj_dict, "/Contents");
    PdfArray* contents_arr = NULL;
    if (contents_ref == -1)
    {
        contents_arr = pdf_dict_get_array(page_obj_dict, "/Contents");
        if (contents_arr == NULL)
        {
            return NULL;
        }
    }
    PdfArray* annots_aar = pdf_dict_get_array(page_obj_dict, "/Annots");
    PdfArray* crop_arr = pdf_dict_get_array(page_obj_dict, "/CropBox");
    PdfArray* media_arr = pdf_dict_get_array(page_obj_dict, "/MediaBox");

    int res_ref = pdf_dict_get_ref(page_obj_dict, "/Resources");
    PdfDict* tmp_dict = NULL;
    if (res_ref == -1)
    {
        PdfDict* res_dict = pdf_dict_get_dict(page_obj_dict, "/Resources");
        if (res_dict == NULL)
            return NULL;
        tmp_dict = res_dict;
    }
    else
    {
        pdf_obj_t* res_obj = pdf_file_get_obj(pdf, res_ref);
        if (res_obj == NULL)
            return NULL;
        tmp_dict = res_obj->value->val.dict;
    }
    pdf_page_t* page = pdf_page_init();
    page->annots = annots_aar;
    page->pageNo = pageNo;
    if (tmp_dict != NULL)
    {
        page->resources = (pdf_resources_t*)malloc(sizeof(pdf_resources_t));
        page->resources->ext_gstate = pdf_dict_get_dict(tmp_dict, "/ExtGState");
        if (page->resources->ext_gstate == NULL)
        {
            int ext_ref = pdf_dict_get_ref(tmp_dict, "/ExtGState");
            if (ext_ref != -1)
            {
                pdf_obj_t* ext_obj = pdf_file_get_obj(pdf, ext_ref);
                if (ext_obj != NULL)
                {
                    page->resources->ext_gstate = ext_obj->value->val.dict;
                }
            }
        }
        page->resources->font_dict = pdf_dict_get_dict(tmp_dict, "/Font");
        if (page->resources->font_dict == NULL)
        {
            int font_ref = pdf_dict_get_ref(tmp_dict, "/Font");
            if (font_ref != -1)
            {
                pdf_obj_t* font_obj = pdf_file_get_obj(pdf, font_ref);
                page->resources->font_dict = font_obj->value->val.dict;
            }
        }
        page->resources->xobject_dict = pdf_dict_get_dict(tmp_dict, "/XObject");
        if (page->resources->xobject_dict == NULL)
        {
            int xobj_ref = pdf_dict_get_ref(tmp_dict, "/XObject");
            if (xobj_ref != -1)
            {
                pdf_obj_t* xobj_obj = pdf_file_get_obj(pdf, xobj_ref);
                page->resources->xobject_dict = xobj_obj->value->val.dict;
            }
        }
    }

    if (contents_ref == -1)
    {
        page->contents = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*) * contents_arr->size());
        page->num_contents = contents_arr->size();
        for (int i = 0; i < contents_arr->size(); i++)
        {
            contents_ref = (*contents_arr)[i]->val.indirect;
            pdf_obj_t* content_obj = pdf_file_get_obj(pdf, contents_ref);
            if (content_obj == NULL)
            {
                pdf_page_free(page);
                return NULL;
            }

            page->contents[i] = content_obj;
        }
    }
    else
    {
        page->num_contents = 1;
        page->contents = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*));
        pdf_obj_t* content_obj = pdf_file_get_obj(pdf, contents_ref);
        if (content_obj == NULL)
        {
            pdf_page_free(page);
            return NULL;
        }
        page->contents[0] = content_obj;
    }


    page->rotate = rotate;
    if (crop_arr != NULL)
    {
        page->crop_box.x = (*crop_arr)[0]->val.number;
        page->crop_box.y = (*crop_arr)[1]->val.number;
        page->crop_box.width = (*crop_arr)[2]->val.number;
        page->crop_box.height = (*crop_arr)[3]->val.number;
    }
    if (media_arr != NULL)
    {
        page->media_box.x = (*media_arr)[0]->val.number;
        page->media_box.y = (*media_arr)[1]->val.number;
        page->media_box.width = (*media_arr)[2]->val.number;
        page->media_box.height = (*media_arr)[3]->val.number;
    }

    page->pdf = pdf;

    return page;
}

bool _add_to_obj_table(pdf_file_t* pdf, pdf_obj_t* obj)
{
    if (pdf == NULL || obj == NULL)
        return false;

    pdf->num_read_objs++;
    if (pdf->read_objs == NULL)
    {
        pdf->read_objs = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*));
        *(pdf->read_objs) = obj;
    }
    else
    {
        pdf_obj_t** o = (pdf_obj_t**)realloc(pdf->read_objs, pdf->num_read_objs * sizeof(pdf_obj_t*));
        if (o == NULL)
        {
            return false;
        }

        pdf->read_objs = o;
        pdf->read_objs[pdf->num_read_objs - 1] = obj;
    }

    return true;
}
pdf_obj_t* _get_obj_from_table(pdf_file_t* pdf, int ref)
{
    if (pdf == NULL || ref < 0)
        return NULL;

    if (pdf->num_read_objs == 0 || pdf->read_objs == NULL)
        return NULL;

    if (pdf->num_read_objs > 0 && pdf->read_objs != NULL)
    {
        for (int i = 0; i < pdf->num_read_objs; i++)
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

pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, int ref)
{
    if (pdf == NULL || ref < 0)
    {
        return NULL;
    }

    pdf_obj_t* ret_obj = _get_obj_from_table(pdf, ref);
    if (ret_obj != NULL)
        return ret_obj;

    if (pdf->xref_table == NULL)
    {
        return NULL;
    }
    int offset = -1;
    for (int i = 0; i < pdf->xref_table->size; i++)
    {
        if (pdf->xref_table->xrefs[i].sequence == ref)
        {
            if (pdf->xref_table->xrefs[i].type == UNCOMPRESSED)
            {
                offset = pdf->xref_table->xrefs[i].uncompressed.offset;
                if (offset == -1)
                {
                    return NULL;
                }
                fseek(pdf->pFile, offset, SEEK_SET);
                pdf_parser_t* parser = pdf_parser_init(pdf, FILE_READER, pdf->pFile);
                pdf_parser_token_t* tk = pdf_parser_next_token(parser);
                if (tk == NULL || tk->type != TOKEN_OBJ_BEG)
                {
                    pdf_parser_token_free(tk);
                    pdf_parser_free(parser);
                    return NULL;
                }
                pdf_obj_t* obj = pdf_parser_build_obj(parser);
                if (obj == NULL)
                {
                    pdf_parser_token_free(tk);
                    pdf_parser_free(parser);
                    return NULL;
                }
                obj->seq = ref;
                obj->pdf = pdf;
                _add_to_obj_table(pdf, obj);
                pdf_parser_token_free(tk);
                pdf_parser_free(parser);
                return obj;
            }
            else
            {
                int obj_ref = pdf->xref_table->xrefs[i].compressed.ref;
                pdf_obj_t* objs_obj = pdf_file_get_obj(pdf, obj_ref);
                if (objs_obj == NULL)
                {
                    return NULL;
                }

                pdf_dict_get_name(objs_obj->value->val.dict, "/Type"); // ObjStm
                int num_pairs = pdf_dict_get_number(objs_obj->value->val.dict, "/N");
                int first_offset = pdf_dict_get_number(objs_obj->value->val.dict, "/First");

                unsigned char* start = NULL;
                int size;
                pdf_stream_get_all(objs_obj->stream, &start, &size);
                pdf_buffer_t b1;
                b1.buffer = start;
                b1.buffer_size = size;
                b1.processed = 0;
                pdf_parser_t* parser = pdf_parser_init(pdf, BUFFER_READER, &b1);
                pdf_parser_token_t* tk = NULL;
                for (int j = 0; j < num_pairs; j++)
                {
                    tk = pdf_parser_next_token(parser);
                    int seq = atoi(tk->token);
                    pdf_parser_token_free(tk);
                    tk = NULL;

                    tk = pdf_parser_next_token(parser);
                    int offset = atoi(tk->token);
                    pdf_parser_token_free(tk);
                    tk = NULL;

                    unsigned char* p1 = start + first_offset + offset;
                    pdf_buffer_t b2;
                    b2.buffer = p1;
                    b2.buffer_size = size - (first_offset + offset);
                    b2.processed = 0;
                    pdf_parser_t* val_parser = pdf_parser_init(pdf, BUFFER_READER, &b2);

                    pdf_parser_token_t* tk1 = pdf_parser_next_token(val_parser);
                    if (tk1 == NULL)
                    {
                        pdf_parser_free(val_parser);
                        return NULL;
                    }
                    pdf_obj_t* obj = pdf_obj_init();
                    obj->seq = seq;
                    obj->value = (pdf_value*)malloc(sizeof(pdf_value));
                    if (tk1->type == TOKEN_DICT_BEG)
                    {
                        PdfDict* obj_dict = pdf_parser_build_dict(val_parser);
                        if (obj_dict == NULL)
                        {
                            pdf_parser_free(val_parser);
                            pdf_parser_token_free(tk1);
                            return NULL;
                        }

                        obj->value->type = DICT;
                        obj->value->val.dict = obj_dict;
                    }
                    else if (tk1->type == TOKEN_ARRAY_BEG)
                    {
                        PdfArray* array = pdf_parser_build_array(val_parser);
                        if (array == NULL)
                        {
                            pdf_parser_free(val_parser);
                            pdf_parser_token_free(tk1);
                            return NULL;
                        }
                        obj->value->type = ARRAY;
                        obj->value->val.array = array;
                    }
                    else
                    {
                        pdf_parser_free(val_parser);
                        pdf_parser_token_free(tk1);
                        return NULL;
                    }
                    pdf_parser_token_free(tk1);
                    pdf_parser_free(val_parser);

                    _add_to_obj_table(pdf, obj);
                }
                pdf_parser_free(parser);

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
    if (pdf->cmaps != NULL)
    {
        pdf_cmap_t* t = pdf->cmaps;
        while (t != NULL)
        {
            if (!strcmp(t->name, name))
            {
                return t;
            }
            t = t->next;
        }
    }
    FILE* f = NULL;
    char filename[256] = {0};
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
    pdf_parser_t* parser = pdf_parser_init(pdf, BUFFER_READER, &b1);
    pdf_cmap_t *cmap = pdf_parser_build_cmap(parser);
    pdf_parser_free(parser);
    free(filedata);
    cmap->worldwide = true;
    cmap->next = NULL;
    strcpy(cmap->name, name);
    if (pdf->cmaps == NULL)
    {
        pdf->cmaps = cmap;
        pdf->num_cmaps = 1;
        return cmap;
    }
    else
    {
        pdf_cmap_t* t = pdf->cmaps;
        while (t->next != NULL)
        {
            t = t->next;
        }
        t->next = cmap;
        pdf->num_cmaps += 1;
        return cmap;
    }
}

void pdf_file_free(pdf_file_t* file)
{
    if (file == NULL) return;

    if (file->xref_table)
    {
        if (file->xref_table->xrefs)
        {
            free(file->xref_table->xrefs);
            file->xref_table->xrefs = NULL;
        }

        free(file->xref_table);
        file->xref_table = NULL;
    }

    if (file->pages)
    {
        free(file->pages);
        file->pages = NULL;
    }

    if (file->num_read_objs > 0 && file->read_objs != NULL)
    {
        for (int i = 0; i < file->num_read_objs; i++)
        {
            pdf_obj_free(file->read_objs[i]);
        }
        free(file->read_objs);
        file->read_objs = NULL;
    }
    if (file->cmaps)
    {
        pdf_cmap_t* p = file->cmaps;
        pdf_cmap_t* q = file->cmaps->next;
        while (q != NULL)
        {
            p->next = q->next;
            pdf_cmap_free(q);
            q = p->next;
        }
        pdf_cmap_free(file->cmaps);
        file->cmaps = NULL;
    }
    if (file->pFile)
    {
        fclose(file->pFile);
        file->pFile = NULL;
    }
    free(file);
    file = NULL;
}