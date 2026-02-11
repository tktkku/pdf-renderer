#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <zlib.h>

int _read_line(pdf_file_t* pdf, char* buf, int size)
{
    if (pdf == NULL || pdf->input == NULL)
        return false;
    memset(buf, 0, size);
    size_t ret = 0;
    char c = '\0';
    int cnt = 0;
    while ((ret = input_read(pdf->input, &c, 1)) > 0 && cnt < size)
    {
        if (c == '\n' || c == '\r')
        {
            while ((ret = input_read(pdf->input, &c, 1)) > 0)
            {
                if (c == '\n' || c == '\r')
                {
                    cnt++;
                }
                else
                {
                    input_seek(pdf->input, -1, SEEK_CUR);
                    break;
                }
            }
            cnt++;
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
    input_seek(pdf->input, 0, SEEK_SET);
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
        xref_t* xref = (xref_t*)malloc(sizeof(xref_t));
        if (xref == NULL)
            return false;
        xref->sequence = seq;

        _read_line(pdf, buffer, sizeof(buffer));
        token = strtok(buffer, " ");
        if (token == NULL) return false;
        xref->type = UNCOMPRESSED;
        xref->uncompressed.offset = strtol(token, NULL, 10);
        token = strtok(NULL, " ");
        if (token == NULL) return false;
        xref->generation = atoi(token);
        token = strtok(NULL, " ");
        if (token == NULL) return false;
        xref->inuse = *token;
        pdf->xref_table.push_back(xref);
    }

    return true;
}

bool _read_xref_and_trailer(pdf_file_t* pdf)
{
    if (pdf == NULL || pdf->input == NULL || pdf->data_len == 0)
    {
        return false;
    }

    char buffer[1024] = { 0 };
    input_seek(pdf->input, pdf->data_len - 128, SEEK_SET);
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
    input_seek(pdf->input, xref_offset, SEEK_SET);
    ret = _read_line(pdf, buffer, sizeof(buffer));
    bool findXref = false;
    input_t* input = NULL;
    input_buffer(&input, buffer, ret);
    pdf_parser_t* parser = pdf_parser_init(pdf, input);
    pdf_parser_token_t* tk = pdf_parser_next_token(parser);
    if (tk == NULL)
    {
        findXref = false;
        input_close(input);
        pdf_parser_token_free(parser, tk);
        pdf_parser_free(parser);
    }
    else if (tk->type == TOKEN_XREF)
    {
        findXref = true;
        input_close(input);
        pdf_parser_token_free(parser, tk);
        pdf_parser_free(parser);
        goto FIND_xref;
    }
    else if (tk->type == TOKEN_OBJ_BEG)
    {
        findXref = true;
        input_close(input);
        pdf_parser_token_free(parser, tk);
        pdf_parser_free(parser);
        goto FIND_XRef;
    }
    else
    {
        input_close(input);
        pdf_parser_token_free(parser, tk);
        pdf_parser_free(parser);
        findXref = false;
    }

    if (!findXref)
    {
        // get wrong xref offset, find from the file start
        input_seek(pdf->input, 0, SEEK_SET);
        int carry = 0;
        while (((ret = input_read(pdf->input, buffer + carry, sizeof(buffer) - carry)) > 0))
        {
            int total = ret + carry;
            for (int i = 0; i < total; i++)
            {
                char c = buffer[i];
                switch (c)
                {
                    case 'x':
                    {
                        if (i + 4 <= total && memcmp(buffer + i, "xref", 4) == 0)
                        {
                            xref_offset = input_tell(pdf->input) - ret + i - carry + 4;
                            input_seek(pdf->input, xref_offset, SEEK_SET);
                            char c1;
                            while (input_read(pdf->input, &c1, 1) == 1)
                            {
                                if (c1 != '\r' && c1 != '\n')
                                {
                                    input_seek(pdf->input, -1, SEEK_CUR);
                                    break;
                                }
                            }
                            findXref = true;
                            goto FIND_xref;
                        }
                        break;
                    }
                    case 'X':
                    {
                        if (i + 4 <= total && memcmp(buffer + i, "XRef", 4) == 0)
                        {
                            // backtrace to find obj start
                            int j = i - 1;
                            for (; j >= 0; j--)
                            {
                                if (j - 3 < 0)
                                {
                                    long cur = input_tell(pdf->input);
                                    if (cur - j >= 0)
                                    {
                                        cur = cur - total + j;
                                        input_seek(pdf->input, cur, SEEK_SET);
                                    }
                                    if (cur >= sizeof(buffer))
                                    {
                                        cur -= (long)sizeof(buffer);
                                    }
                                    else
                                    {
                                        cur = 0;
                                    }
                                    input_seek(pdf->input, cur, SEEK_SET);
                                    ret = input_read(pdf->input, buffer, sizeof(buffer));
                                    j = sizeof(buffer) - 1;
                                }
                                char c1 = buffer[j];
                                switch (c1)
                                {
                                    case 'j':
                                    {
                                        if (memcmp(buffer + j - 3 + 1, "obj", 3) == 0)
                                        {
                                            xref_offset = input_tell(pdf->input) - ret + j + 1;
                                            input_seek(pdf->input, xref_offset, SEEK_SET);
                                            findXref = true;
                                            goto FIND_XRef;
                                        }
                                        break;
                                    }
                                    default:
                                    break;
                                }
                            }
                            break;
                        }
                        break;
                    }
                    default:
                        break;
                }
                if (findXref)
                {
                    break;
                }
            }
            if (findXref)
            {
                break;
            }
            if (total >= 4)
            {
                carry = 4;
                memmove(buffer, buffer + total - 4, 4);
            }
            else
            {
                carry = total;
                memmove(buffer, buffer + total - carry, carry);
            }
        }
    }
    if (!findXref)
    {
        printf("invalid pdf, xref not found\n");
        return false;
    }

FIND_xref: 
    {
        while (true)
        {
            ret = _read_line(pdf, buffer, sizeof(buffer)); // n n
            if (memcmp(buffer, "trailer", 7) == 0)
            {
                if (ret != 7)
                    input_seek(pdf->input, -ret + 7, SEEK_CUR);
                break;
            }
            input_seek(pdf->input, -ret, SEEK_CUR);
            if (!_read_xref_table(pdf))
            {
                return false;
            }

        }
        // read trailer
        pdf_parser_t* parser = pdf_parser_init(pdf, pdf->input);
        pdf_parser_token_t* tk = pdf_parser_next_token(parser);
        if (tk == NULL || tk->type != TOKEN_DICT_BEG)
        {
            pdf_parser_token_free(parser, tk);
            tk = NULL;
            pdf_parser_free(parser);
            return false;
        }
        pdf_parser_token_free(parser, tk);
        tk = NULL;
        pdf_dict_t* trailer = pdf_parser_build_dict(parser);
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
                input_seek(pdf->input, pre_offset, SEEK_SET);
                _read_line(pdf, buffer, sizeof(buffer));// xref skip this line
                if (!_read_xref_table(pdf))
                {
                    return false;
                }
            }
        }
        pdf_dict_free(trailer);
        pdf_parser_free(parser);
        return true;
    }
FIND_XRef:
    {
        pdf_parser_t* parser = pdf_parser_init(pdf, pdf->input);
        pdf_obj_t* xref_obj = pdf_parser_build_obj(parser);
        pdf_parser_free(parser);
        if (xref_obj == NULL || xref_obj->value->type != PDF_VALUE_DICT)
        {
            return false;
        }
        pdf_dict_t* xref_dict = xref_obj->value->val.dict;
        pdf->info_obj_ref = pdf_dict_get_ref(xref_dict, "/Info");
        pdf->root_obj_ref = pdf_dict_get_ref(xref_dict, "/Root");

        pdf_dict_get_name(xref_dict, "/Type"); // XRef
        int size = pdf_dict_get_number(xref_dict, "/Size");
        pdf_array_t* index_arr = pdf_dict_get_array(xref_dict, "/Index");
        pdf_array_t* w_aar = pdf_dict_get_array(xref_dict, "/W");
        if (index_arr == NULL)
        {
            index_arr = (pdf_array_t*)malloc(sizeof(pdf_array_t));
            index_arr->num_elements = 2;
            index_arr->values = (pdf_array_element_value_t**)malloc(index_arr->num_elements * sizeof(pdf_array_element_value_t*));
            index_arr->values[0] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            index_arr->values[0]->type = PDF_VALUE_NUMBER;
            index_arr->values[0]->val.number = 0;
            index_arr->values[1] = (pdf_array_element_value_t*)malloc(sizeof(pdf_array_element_value_t));
            index_arr->values[1]->type = PDF_VALUE_NUMBER;
            index_arr->values[1]->val.number = size;

            pdf_dict_add_array(xref_dict, "/Index", index_arr);
        }
        if (w_aar == NULL)
        {
            return false;
        }

        int w0 = w_aar->values[0]->val.number;
        int w1 = w_aar->values[1]->val.number;
        int w2 = w_aar->values[2]->val.number;

        unsigned char* start = NULL;
        int len;
        pdf_stream_get_all(xref_obj->stream, &start, &len);
        if (start != NULL)
        {
            unsigned char* origin = start;
            for (int i = 0; i < index_arr->num_elements; i += 2)
            {
                int start_index = index_arr->values[i]->val.number;
                int num = index_arr->values[i + 1]->val.number;

                int seq = start_index;

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
                    xref_t* xref = (xref_t*)malloc(sizeof(xref_t));
                    if (xref == NULL)
                        return false;
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
            free(origin);
        }
        pdf_obj_free(xref_obj);
        return true;
    }
}
void _read_pages(pdf_file_t* pdf, pdf_obj_t* pages_obj);
void _read_pages(pdf_file_t* pdf, pdf_obj_t* pages_obj)
{
    if (pdf == NULL || pages_obj == NULL || pages_obj->value->type != PDF_VALUE_DICT) return;

    const char* type = pdf_dict_get_name(pages_obj->value->val.dict, "/Type");
    int count = pdf_dict_get_number(pages_obj->value->val.dict, "/Count");
    pdf_array_t* kids_arr = pdf_dict_get_array(pages_obj->value->val.dict, "/Kids");
    if (kids_arr == NULL) return;
    for (int i = 0; i < kids_arr->num_elements; i++)
    {
        int ref = kids_arr->values[i]->val.indirect;
        pdf_obj_t* obj = pdf_file_get_obj(pdf, ref);
        if (obj == NULL) continue;
        type = pdf_dict_get_name(obj->value->val.dict, "/Type");
        if (type == NULL) continue;
        if (!strcmp(type, "/Pages"))
        {
            _read_pages(pdf, obj);
        }
        else
        {
           pdf->pages.push_back(obj);
        }
    } 
}
pdf_file_t* _fill_pdf_file(pdf_file_t* pdf)
{
    pdf->current_index = 0;

    if (!_check_version(pdf))
    {
        pdf_file_free(pdf);
        return NULL;
    }
    if (!_read_xref_and_trailer(pdf))
    {
        pdf_file_free(pdf);
        return NULL;
    }

    pdf_obj_t* root_obj = pdf_file_get_obj(pdf, pdf->root_obj_ref);
    if (root_obj == NULL || root_obj->value->type != PDF_VALUE_DICT)
    {
        pdf_file_free(pdf);
        return NULL;
    }
    pdf_dict_t* names_dict = pdf_dict_get_dict(root_obj->value->val.dict, "/Names");
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
            pdf_obj_t* embedded_obj1 = pdf_file_get_obj(pdf, embedded_ref);
            if (embedded_obj1 != NULL)
            {
                pdf_array_t* names_aar = pdf_dict_get_array(embedded_obj1->value->val.dict, "/Names");
                if (names_aar != NULL)
                {
                    for (int i = 0; i < names_aar->num_elements; i++)
                    {
                        if (names_aar->values[i]->type == PDF_VALUE_INDIRECT)
                        {
                            pdf_obj_t* embedded_obj2 = pdf_file_get_obj(pdf, names_aar->values[i]->val.indirect);
                            if (embedded_obj2 != NULL)
                            {
                                pdf_dict_t* ef_dict = pdf_dict_get_dict(embedded_obj2->value->val.dict, "/EF");
                                int ref = pdf_dict_get_ref(ef_dict, "/UF");
                                pdf_obj_t* embedded_obj = pdf_file_get_obj(pdf, ref);
                                unsigned char* embedded_file = NULL;
                                int embedded_file_len = 0;
                                pdf_stream_get_all(embedded_obj->stream, &embedded_file, &embedded_file_len);
                                embedded_file_len += 1;
                                free(embedded_file);
                            }
                        }
                    }
                    
                }
            }
        }
        // /AlternatePresentations
        // /Renditions
    }
    pdf_dict_t* acroform_dict = pdf_dict_get_dict(root_obj->value->val.dict, "/AcroForm");
    if (acroform_dict != NULL)
    {

    }

    int ref = pdf_dict_get_ref(root_obj->value->val.dict, "/Pages");
    pdf_obj_t* pages_obj = pdf_file_get_obj(pdf, ref);
    if (pages_obj == NULL)
    {
        pdf_file_free(pdf);
        return NULL;
    }

    _read_pages(pdf, pages_obj);
    return pdf;
}
pdf_file_t* pdf_file_read_buffer(const char* data, size_t size)
{
    if (data == NULL || size <= 0) return NULL;
    input_t* input = NULL;
    if (input_buffer(&input, data, size) != 0)
    {
        return NULL;
    }
    pdf_file_t* pdf_file = new pdf_file_t;
    if (pdf_file == NULL)
        return NULL;
    memset(pdf_file, 0, sizeof(pdf_file_t));
    
    pdf_file->input = input;
    pdf_file->data_len = size;

    return _fill_pdf_file(pdf_file);
}
pdf_file_t* pdf_file_read_file(const char* file_name)
{
    if (file_name == NULL)
    {
        return NULL;
    }
    
    input_t* input = NULL;
    if (input_file(&input, file_name) != 0)
    {
        return NULL;
    }
    pdf_file_t* pdf_file = new pdf_file_t;
    if (pdf_file == NULL)
        return NULL;
    memset(pdf_file, 0, sizeof(pdf_file_t));
    
    pdf_file->input = input;
    input_seek(pdf_file->input, 0, SEEK_END);
    long file_size = input_tell(pdf_file->input);
    input_seek(pdf_file->input, 0, SEEK_SET);
    pdf_file->data_len = file_size;

    return _fill_pdf_file(pdf_file);
}
int pdf_file_get_pages(pdf_file_t* pdf)
{
    if (pdf == NULL) return 0;
    return pdf->pages.size();
}
pdf_page_t* pdf_file_get_page(pdf_file_t* pdf, int pageNo)
{
    if (pdf == NULL)
    {
        return NULL;
    }

    pdf_obj_t* page_obj = pdf->pages[pageNo];
    pdf_dict_t* page_obj_dict = page_obj->value->val.dict;
    const char* type = pdf_dict_get_name(page_obj_dict, "/Type");
    if (strcmp(type, "/Page") != 0)
    {
        return NULL;
    }

    int rotate = pdf_dict_get_number(page_obj_dict, "/Rotate");

    int contents_ref = pdf_dict_get_ref(page_obj_dict, "/Contents");
    pdf_array_t* contents_arr = NULL;
    if (contents_ref == -1)
    {
        contents_arr = pdf_dict_get_array(page_obj_dict, "/Contents");
        if (contents_arr == NULL)
        {
            return NULL;
        }
    }
    pdf_array_t* annots_aar = pdf_dict_get_array(page_obj_dict, "/Annots");
    pdf_array_t* crop_arr = pdf_dict_get_array(page_obj_dict, "/CropBox");
    pdf_array_t* media_arr = pdf_dict_get_array(page_obj_dict, "/MediaBox");

    // int res_ref = pdf_dict_get_ref(page_obj_dict, "/Resources");
    // pdf_dict_t* tmp_dict = NULL;
    // if (res_ref == -1)
    // {
    //     pdf_dict_t* res_dict = pdf_dict_get_dict(page_obj_dict, "/Resources");
    //     if (res_dict == NULL)
    //         return NULL;
    //     tmp_dict = res_dict;
    // }
    // else
    // {cvector_size
    //     pdf_obj_t* res_obj = pdf_file_get_obj(pdf, res_ref);
    //     if (res_obj == NULL)
    //         return NULL;
    //     tmp_dict = res_obj->value->val.dict;
    // }
    pdf_page_t* page = pdf_page_init();
    page->annots = annots_aar;
    page->pageNo = pageNo;
    page->obj = page_obj;
    // if (tmp_dict != NULL)
    // {
    //     page->resources = (pdf_resources_t*)malloc(sizeof(pdf_resources_t));
    //     page->resources->ext_gstate = pdf_dict_get_dict(tmp_dict, "/ExtGState");
    //     if (page->resources->ext_gstate == NULL)
    //     {
    //         int ext_ref = pdf_dict_get_ref(tmp_dict, "/ExtGState");
    //         if (ext_ref != -1)
    //         {
    //             pdf_obj_t* ext_obj = pdf_file_get_obj(pdf, ext_ref);
    //             if (ext_obj != NULL)
    //             {
    //                 page->resources->ext_gstate = ext_obj->value->val.dict;
    //             }
    //         }
    //     }
    //     page->resources->font_dict = pdf_dict_get_dict(tmp_dict, "/Font");
    //     if (page->resources->font_dict == NULL)
    //     {
    //         int font_ref = pdf_dict_get_ref(tmp_dict, "/Font");
    //         if (font_ref != -1)
    //         {
    //             pdf_obj_t* font_obj = pdf_file_get_obj(pdf, font_ref);
    //             page->resources->font_dict = font_obj->value->val.dict;
    //         }
    //     }
    //     page->resources->xobject_dict = pdf_dict_get_dict(tmp_dict, "/XObject");
    //     if (page->resources->xobject_dict == NULL)
    //     {
    //         int xobj_ref = pdf_dict_get_ref(tmp_dict, "/XObject");
    //         if (xobj_ref != -1)
    //         {
    //             pdf_obj_t* xobj_obj = pdf_file_get_obj(pdf, xobj_ref);
    //             page->resources->xobject_dict = xobj_obj->value->val.dict;
    //         }
    //     }
    // }

    if (contents_ref == -1)
    {
        page->contents = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*) * contents_arr->num_elements);
        page->num_contents = contents_arr->num_elements;
        for (int i = 0; i < contents_arr->num_elements; i++)
        {
            contents_ref = contents_arr->values[i]->val.indirect;
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

    if (rotate == -1)
    {
        rotate = 0;
    }
    page->rotate = rotate;
    if (crop_arr != NULL)
    {
        page->crop_box.x = crop_arr->values[0]->val.number;
        page->crop_box.y = crop_arr->values[1]->val.number;
        page->crop_box.width = crop_arr->values[2]->val.number;
        page->crop_box.height = crop_arr->values[3]->val.number;
    }
    if (media_arr != NULL)
    {
        page->media_box.x = media_arr->values[0]->val.number;
        page->media_box.y = media_arr->values[1]->val.number;
        page->media_box.width = media_arr->values[2]->val.number;
        page->media_box.height = media_arr->values[3]->val.number;
    }

    page->pdf = pdf;

    return page;
}

pdf_obj_t* _get_obj_from_table(pdf_file_t* pdf, int ref)
{
    if (pdf == NULL || ref < 0)
        return NULL;
    int nums = pdf->read_objs.size();
    for (int i = 0; i < nums; i++)
    {
        if (pdf->read_objs[i]->seq == ref)
        {
            pdf->read_objs[i]->pdf = pdf;
            return pdf->read_objs[i];
        }
    }

    return NULL;
}
void _fill_resources(pdf_obj_t* obj)
{
    if (obj->value == NULL || obj->value->type != PDF_VALUE_DICT) return;
    int ref = pdf_dict_get_ref(obj->value->val.dict, "/Resources");
    pdf_dict_t* resources = NULL;
    if (ref == -1)
    {
        resources = pdf_dict_get_dict(obj->value->val.dict, "/Resources");
    }
    else
    {
        pdf_obj_t* res_obj = pdf_file_get_obj(obj->pdf, ref);
        if (res_obj == NULL || res_obj->value->type != PDF_VALUE_DICT)
            return;
        resources = res_obj->value->val.dict;
    }
    if (resources == NULL)
        return;
    obj->resources.extgstate_dict = pdf_dict_get_dict(resources, "/ExtGState");
    if (obj->resources.extgstate_dict == NULL)
    {
        int ext_ref = pdf_dict_get_ref(resources, "/ExtGState");
        if (ext_ref != -1)
        {
            pdf_obj_t* ext_obj = pdf_file_get_obj(obj->pdf, ext_ref);
            if (ext_obj != NULL)
            {
                obj->resources.extgstate_dict = ext_obj->value->val.dict;
            }
        }
    }
    obj->resources.colorspace_dict = pdf_dict_get_dict(resources, "/ColorSpace");
    if (obj->resources.colorspace_dict == NULL)
    {
        int color_ref = pdf_dict_get_ref(resources, "/ColorSpace");
        if (color_ref != -1)
        {
            pdf_obj_t* color_obj = pdf_file_get_obj(obj->pdf, color_ref);
            if (color_obj != NULL)
            {
                obj->resources.colorspace_dict = color_obj->value->val.dict;
            }
        }
    }
    obj->resources.pattern_dict = pdf_dict_get_dict(resources, "/Pattern");
    if (obj->resources.pattern_dict == NULL)
    {
        int pattern_ref = pdf_dict_get_ref(resources, "/Pattern");
        if (pattern_ref != -1)
        {
            pdf_obj_t* pattern_obj = pdf_file_get_obj(obj->pdf, pattern_ref);
            if (pattern_obj != NULL)
            {
                obj->resources.pattern_dict = pattern_obj->value->val.dict;
            }
        }
    }
    obj->resources.shading_dict = pdf_dict_get_dict(resources, "/Shading");
    if (obj->resources.shading_dict == NULL)
    {
        int shading_ref = pdf_dict_get_ref(resources, "/Shading");
        if (shading_ref != -1)
        {
            pdf_obj_t* shading_obj = pdf_file_get_obj(obj->pdf, shading_ref);
            if (shading_obj != NULL)
            {
                obj->resources.shading_dict = shading_obj->value->val.dict;
            }
        }
    }
    obj->resources.xobject_dict = pdf_dict_get_dict(resources, "/XObject");
    if (obj->resources.xobject_dict == NULL)
    {
        int xobj_ref = pdf_dict_get_ref(resources, "/XObject");
        if (xobj_ref != -1)
        {
            pdf_obj_t* xobj_obj = pdf_file_get_obj(obj->pdf, xobj_ref);
            if (xobj_obj != NULL)
            {
                obj->resources.xobject_dict = xobj_obj->value->val.dict;
            }
        }
    }
    obj->resources.font_dict = pdf_dict_get_dict(resources, "/Font");
    if (obj->resources.font_dict == NULL)
    {
        int font_ref = pdf_dict_get_ref(resources, "/Font");
        if (font_ref != -1)
        {
            pdf_obj_t* font_obj = pdf_file_get_obj(obj->pdf, font_ref);
            if (font_obj != NULL)
            {
                obj->resources.font_dict = font_obj->value->val.dict;
            }
        }
    }
    obj->resources.procset_arr = pdf_dict_get_array(resources, "/ProcSet");
    if (obj->resources.procset_arr == NULL)
    {
        int proc_ref = pdf_dict_get_ref(resources, "/ProcSet");
        if (proc_ref != -1)
        {
            pdf_obj_t* proc_obj = pdf_file_get_obj(obj->pdf, proc_ref);
            if (proc_obj != NULL)
            {
                obj->resources.procset_arr = proc_obj->value->val.array;
            }
        }
    }
    obj->resources.properties_dict = pdf_dict_get_dict(resources, "/Properties");
    if (obj->resources.properties_dict == NULL)
    {
        int prop_ref = pdf_dict_get_ref(resources, "/Properties");
        if (prop_ref != -1)
        {
            pdf_obj_t* prop_obj = pdf_file_get_obj(obj->pdf, prop_ref);
            if (prop_obj != NULL)
            {
                obj->resources.properties_dict = prop_obj->value->val.dict;
            }
        }
    }
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

    if (pdf->xref_table.empty())
    {
        return NULL;
    }
    int offset = -1;
    int num_xref = pdf->xref_table.size();
    for (int i = 0; i < num_xref; i++)
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
                input_seek(pdf->input, offset, SEEK_SET);
                pdf_parser_t* parser = pdf_parser_init(pdf, pdf->input);
                pdf_parser_token_t* tk = pdf_parser_next_token(parser);
                if (tk == NULL || tk->type != TOKEN_OBJ_BEG)
                {
                    pdf_parser_token_free(parser, tk);
                    pdf_parser_free(parser);
                    return NULL;
                }
                pdf_obj_t* obj = pdf_parser_build_obj(parser);
                if (obj == NULL)
                {
                    pdf_parser_token_free(parser, tk);
                    pdf_parser_free(parser);
                    return NULL;
                }
                obj->seq = ref;
                obj->pdf = pdf;
                _fill_resources(obj);
                pdf->read_objs.push_back(obj);
                pdf_parser_token_free(parser, tk);
                pdf_parser_free(parser);
                return obj;
            }
            else
            {
                int obj_ref = pdf->xref_table[i]->compressed.ref;
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
                if (start == NULL) return NULL;
                input_t* input = NULL;
                input_buffer(&input, (char*)start, size);
                unsigned char* origin = start;
                pdf_parser_t* parser = pdf_parser_init(pdf, input);
                pdf_parser_token_t* tk = NULL;
                for (int j = 0; j < num_pairs; j++)
                {
                    tk = pdf_parser_next_token(parser);
                    int seq = atoi(tk->token.data());
                    pdf_parser_token_free(parser, tk);
                    tk = NULL;

                    tk = pdf_parser_next_token(parser);
                    int offset = atoi(tk->token.data());
                    pdf_parser_token_free(parser, tk);
                    tk = NULL;

                    unsigned char* p1 = start + first_offset + offset;
                    input_t* input2 = NULL;
                    input_buffer(&input2, (char*)p1, size - first_offset - offset);
                    pdf_parser_t* val_parser = pdf_parser_init(pdf, input2);

                    pdf_parser_token_t* tk1 = pdf_parser_next_token(val_parser);
                    if (tk1 == NULL)
                    {
                        pdf_parser_free(val_parser);
                        return NULL;
                    }
                    pdf_obj_t* obj = pdf_obj_init();
                    obj->seq = seq;
                    obj->value = (pdf_obj_value_t*)malloc(sizeof(pdf_obj_value_t));
                    if (tk1->type == TOKEN_DICT_BEG)
                    {
                        pdf_dict_t* obj_dict = pdf_parser_build_dict(val_parser);
                        if (obj_dict == NULL)
                        {
                            pdf_parser_free(val_parser);
                            pdf_parser_token_free(parser, tk1);
                            return NULL;
                        }

                        obj->value->type = PDF_VALUE_DICT;
                        obj->value->val.dict = obj_dict;
                    }
                    else if (tk1->type == TOKEN_ARRAY_BEG)
                    {
                        pdf_array_t* array = pdf_parser_build_array(val_parser);
                        if (array == NULL)
                        {
                            pdf_parser_free(val_parser);
                            pdf_parser_token_free(parser, tk1);
                            return NULL;
                        }
                        obj->value->type = PDF_VALUE_ARRAY;
                        obj->value->val.array = array;
                    }
                    else
                    {
                        pdf_parser_free(val_parser);
                        pdf_parser_token_free(parser, tk1);
                        return NULL;
                    }
                    pdf_parser_token_free(parser, tk1);
                    pdf_parser_free(val_parser);
                    input_close(input2);
                    _fill_resources(obj);
                    pdf->read_objs.push_back(obj);
                }
                free(origin);  
                pdf_parser_free(parser);
                input_close(input);
                return _get_obj_from_table(pdf, ref);
            }
        }
    }
    return NULL;
}
void pdf_file_load_font(pdf_file_t* file, const char* name, const char* data, long len)
{
    if (file == NULL || name == NULL || data == NULL || len <= 0)
        return;
    pdf_external_font_t* font = (pdf_external_font_t*)malloc(sizeof(pdf_external_font_t));
    font->name_len = strlen(name);
    font->name = (char*)malloc(font->name_len + 1);
    memcpy(font->name, name, font->name_len);
    font->name[font->name_len] = '\0';
    
    font->data_len = len;
    font->data = (char*)malloc(font->data_len);
    memcpy(font->data, data, font->data_len);
    
    file->external_fonts.push_back(font);
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
    input_t* input = NULL;
    input_buffer(&input, (char*)filedata, filesize);
    pdf_parser_t* parser = pdf_parser_init(pdf, input);
    pdf_cmap_t* cmap = pdf_parser_build_cmap(parser);
    pdf_parser_free(parser);
    input_close(input);
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
    int nums = file->xref_table.size();
    for (int i = 0; i < nums; i++)
    {
        free(file->xref_table[i]);
    }
    file->xref_table.clear();

    file->pages.clear();
    nums = file->read_objs.size();
    for (int i = 0; i < nums; i++)
    {
        pdf_obj_free(file->read_objs[i]);
    }
    file->read_objs.clear();
    nums = file->external_fonts.size();
    for (int i = 0; i < nums; i++)
    {
        pdf_external_font_t* f = file->external_fonts[i];
        if (f == NULL) continue;
        if (f->data)
        {
            free(f->data);
            f->data = NULL;
        }
        if (f->name)
        {
            free(f->name);
            f->name = NULL;
        }
        free(f);
        f = NULL;
    }

    file->external_fonts.clear();
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
    // if (file->freed_tokens)
    // {
    //     pdf_parser_token_t* p = file->freed_tokens;
    //     pdf_parser_token_t* q = file->freed_tokens->next;
    //     while (q != NULL)
    //     {
    //         p->next = q->next;
    //         pdf_parser_token_free(NULL, q);
    //         q = p->next;
    //     }
    //     pdf_parser_token_free(NULL, file->freed_tokens);
    //     file->freed_tokens = NULL;
    // }
    if (file->input)
    {
        input_close(file->input);
        file->input = NULL;
    }
    delete file;
    file = NULL;
}