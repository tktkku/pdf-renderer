#include "pdf.h"
#include "pdf-private.h"

#include <stdint.h>
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
    pdf_token* tk = pdf_parser_next_token(parser);
    if (tk == NULL)
    {
        findXref = false;
        input_close(input);
        delete tk;
        pdf_parser_free(parser);
    }
    else if (tk->type() == TOKEN_XREF)
    {
        findXref = true;
        input_close(input);
        delete tk;
        pdf_parser_free(parser);
        goto FIND_xref;
    }
    else if (tk->type() == TOKEN_OBJ_BEG)
    {
        findXref = true;
        input_close(input);
        delete tk;
        pdf_parser_free(parser);
        goto FIND_XRef;
    }
    else
    {
        input_close(input);
        delete tk;
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
                                    long unsigned int cur = input_tell(pdf->input);
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
        pdf_token* tk = pdf_parser_next_token(parser);
        if (tk == NULL || tk->type() != TOKEN_DICT_BEG)
        {
            delete tk;
            tk = NULL;
            pdf_parser_free(parser);
            return false;
        }
        delete tk;
        tk = NULL;
        pdf_dict* trailer = pdf_parser_build_dict(parser);
        if (trailer == NULL)
        {
            pdf_parser_free(parser);
            return false;
        }
        else
        {
            pdf->trailer = trailer;
            pdf->id_arr = trailer->get_array("/ID");
            pdf->root_obj_ref = trailer->get_indirect("/Root");
            pdf->info_obj_ref = trailer->get_indirect("/Info");
            pdf->encrypt_obj_ref = trailer->get_indirect("/Encrypt");

            if (trailer->has("/Prev"))
            {
                int pre_offset = trailer->get_number("/Prev");
                pdf->current_index = pre_offset;
                input_seek(pdf->input, pre_offset, SEEK_SET);
                _read_line(pdf, buffer, sizeof(buffer));// xref skip this line
                if (!_read_xref_table(pdf))
                {
                    return false;
                }
            }
        }
        
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
        pdf_dict* xref_dict = xref_obj->value->val.dict;
        pdf->id_arr = xref_dict->get_array("/ID");
        pdf->info_obj_ref = xref_dict->get_indirect("/Info");
        pdf->root_obj_ref = xref_dict->get_indirect("/Root");
        pdf->encrypt_obj_ref = xref_dict->get_indirect("/Encrypt");

        int size = xref_dict->get_number("/Size");
        pdf_array* index_arr = NULL;
        if (xref_dict->has("/Index"))
            index_arr = xref_dict->get_array("/Index");
        else
        {
            index_arr = new pdf_array();

            pdf_value_t* value0 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            value0->type = PDF_VALUE_NUMBER;
            value0->val.number = 0;
            index_arr->add(value0);

            pdf_value_t* value1 = (pdf_value_t*)malloc(sizeof(pdf_value_t));
            value1->type = PDF_VALUE_NUMBER;
            value1->val.number = size;
            index_arr->add(value1);

            xref_dict->add("/Index", PDF_VALUE_ARRAY, index_arr);
        }
        if (!xref_dict->has("/W"))
        {
            return false;
        }
        pdf_array* w_aar = xref_dict->get_array("/W");

        int w0 = w_aar->get(0)->val.number;
        int w1 = w_aar->get(1)->val.number;
        int w2 = w_aar->get(2)->val.number;

        unsigned char* start = NULL;
        int len;
        pdf_stream_get_all(xref_obj->stream, &start, &len);
        if (start != NULL)
        {
            unsigned char* origin = start;
            for (size_t i = 0; i < index_arr->size(); i += 2)
            {
                int start_index = index_arr->get(i)->val.number;
                int num = index_arr->get(i + 1)->val.number;

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
    pdf_dict* dict = pages_obj->value->val.dict;
    const char* type = dict->get_name("/Type");
    int count = dict->get_number("/Count");
    if (!dict->has("/Kids"))
    {
        return;
    }
    pdf_array* kids_arr = dict->get_array("/Kids");
    if (kids_arr == NULL) return;
    for (size_t i = 0; i < kids_arr->size(); i++)
    {
        pdf_indirect_t ref = kids_arr->get(i)->val.indirect;
        pdf_obj_t* obj = pdf_file_get_obj(pdf, ref);
        if (obj == NULL) continue;
        if (!obj->value->val.dict->has("/Type"))
        {
            continue;
        }
        type = obj->value->val.dict->get_name("/Type");
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
    if (pdf->encrypt_obj_ref.obj_num != -1)
    {
        pdf_obj_t* obj = pdf_file_get_obj(pdf, pdf->encrypt_obj_ref);
        if (obj != NULL)
        {
            pdf_dict* dict = obj->value->val.dict;
            uint8_t padding[32] = { 
                0x28,0xBF,0x4E,0x5E,0x4E,0x75,0x8A,0x41,
                0x64,0x00,0x4E,0x56,0xFF,0xFA,0x01,0x08,
                0x2E,0x2E,0x00,0xB6,0xD0,0x68,0x3E,0x80,
                0x2F,0x0C,0xA9,0xFE,0x64,0x53,0x69,0x7A
            };
            const char* password = "";
            uint8_t pw_padded[32] = { 0 };
            int pw_len = strlen(password);
            if (pw_len > 32) pw_len = 32;
            memcpy(pw_padded, password, pw_len);
            if (pw_len < 32)
                memcpy(pw_padded + pw_len, padding, 32 - pw_len);

            const char* filter = dict->get_name("/Filter");
            int V = dict->get_number("/V");
            int R = dict->get_number("/R");
            pdf->encrypt_key_len_bits = dict->get_number("/Length");
            int32_t P = dict->get_number("/P");
            const char* o = dict->get_string("/O");
            int o_l = strlen(o);
            std::vector<uint8_t> o_vec;
            for (int i = 1; i < o_l; i += 2)
            {
                o_vec.push_back(_hex_str_to_8bit(o + i, 2));
            }
            const char* U = dict->get_string("/U");
            std::vector<uint8_t> id_vec;
            for (int i = 1; i < pdf->id_arr->get(0)->value_len; i += 2)
            {
                id_vec.push_back(_hex_str_to_8bit(pdf->id_arr->get(0)->val.string + i, 2));
            }
            std::vector<uint8_t> vec;
            vec.insert(vec.end(), pw_padded, pw_padded + 32);
            vec.insert(vec.end(), o_vec.begin(), o_vec.end());
            vec.push_back(((P >> 0)     & 0xFF));
            vec.push_back(((P >> 8)     & 0xFF));
            vec.push_back(((P >> 16)    & 0xFF));
            vec.push_back(((P >> 24)    & 0xFF));
            vec.insert(vec.end(), id_vec.begin(), id_vec.end());
            if (R >= 4)
            {
                vec.push_back(0xFF); vec.push_back(0xFF); vec.push_back(0xFF); vec.push_back(0xFF);
            }
            uint8_t* data = new uint8_t[vec.size()];
            memcpy(data, vec.data(), vec.size());

            uint8_t hash[16] = { 0 };
            md5(data, vec.size(), hash);
            delete[] data;

            uint8_t key[16] = { 0 };
            memcpy(key, hash, pdf->encrypt_key_len_bits / 8);
            if (R >= 3)
            {
                for (int i = 0; i < 50; i++)
                {
                    md5(key, pdf->encrypt_key_len_bits / 8, hash);
                    memcpy(key, hash, pdf->encrypt_key_len_bits / 8);
                }
            }
            memset(pdf->encrypt_key, 0, sizeof(pdf->encrypt_key));
            memcpy(pdf->encrypt_key, key, pdf->encrypt_key_len_bits / 8);
        }
    }
    
    if (root_obj->value->val.dict->has("/Names"))
    {
        // /Dests
        // /AP
        // /JavaScript
        // /Pages
        // /Templates
        // /IDS
        // /URLS
        // /EmbeddedFiles
        
        if (root_obj->value->val.dict->is_dict("/Names"))
        {
            pdf_dict* names_dict = root_obj->value->val.dict->get_dict("/Names");
            if (names_dict->has("/EmbeddedFiles"))
            {
                if (names_dict->is_indirect("/EmbeddedFiles"))
                {
                    pdf_indirect_t embedded_ref = names_dict->get_indirect("/EmbeddedFiles"); 
                    pdf_obj_t* embedded_obj1 = pdf_file_get_obj(pdf, embedded_ref);
                    if (embedded_obj1 != NULL)
                    { 
                        if (embedded_obj1->value->val.dict->has("/Names"))
                        {
                            pdf_array* names_aar = embedded_obj1->value->val.dict->get_array("/Names");
                            for (size_t i = 0; i < names_aar->size(); i++)
                            {
                                if (names_aar->get(i)->type == PDF_VALUE_INDIRECT)
                                {
                                    pdf_obj_t* embedded_obj2 = pdf_file_get_obj(pdf, names_aar->get(i)->val.indirect);
                                    if (embedded_obj2 != NULL)
                                    {
                                        pdf_dict* ef_dict = embedded_obj2->value->val.dict->get_dict("/EF");
                                        pdf_indirect_t ref = ef_dict->get_indirect("/UF");
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
            }
        }
        // /AlternatePresentations
        // /Renditions
    }
      
    if (root_obj->value->val.dict->has("/AcroForm"))
    {
        pdf_dict* acroform_dict = root_obj->value->val.dict->get_dict("/AcroForm");
    }

    pdf_indirect_t ref = root_obj->value->val.dict->get_indirect("/Pages");
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
    pdf_dict* page_obj_dict = page_obj->value->val.dict;
    const char* type = page_obj_dict->get_name("/Type");
    if (strcmp(type, "/Page") != 0)
    {
        return NULL;
    }

    int rotate = page_obj_dict->get_number("/Rotate");

    pdf_array* annots_aar = page_obj_dict->get_array("/Annots");
    pdf_array* crop_arr = page_obj_dict->get_array("/CropBox");
    pdf_array* media_arr = page_obj_dict->get_array("/MediaBox");

    pdf_page_t* page = pdf_page_init();
    page->annots = annots_aar;
    page->pageNo = pageNo;
    page->obj = page_obj;

    pdf_array* contents_arr = NULL;
    if (page_obj_dict->has("/Contents"))
    {
        if (page_obj_dict->is_indirect("/Contents"))
        {
            pdf_indirect_t contents_ref = page_obj_dict->get_indirect("/Contents");
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
        else if (page_obj_dict->is_array("/Contents"))
        {
            contents_arr = page_obj_dict->get_array("/Contents");
            page->contents = (pdf_obj_t**)malloc(sizeof(pdf_obj_t*) * contents_arr->size());
            page->num_contents = contents_arr->size();
            for (size_t i = 0; i < contents_arr->size(); i++)
            {
                pdf_indirect_t contents_ref = contents_arr->get(i)->val.indirect;
                pdf_obj_t* content_obj = pdf_file_get_obj(pdf, contents_ref);
                if (content_obj == NULL)
                {
                    pdf_page_free(page);
                    return NULL;
                }

                page->contents[i] = content_obj;
            }
        }
    }
    else
    {
        pdf_page_free(page);
        return NULL;
    }

    if (rotate == -1)
    {
        rotate = 0;
    }
    page->rotate = rotate;
    if (crop_arr != NULL)
    {
        page->crop_box.x = crop_arr->get(0)->val.number;
        page->crop_box.y = crop_arr->get(1)->val.number;
        page->crop_box.width = crop_arr->get(2)->val.number;
        page->crop_box.height = crop_arr->get(3)->val.number;
    }
    if (media_arr != NULL)
    {
        page->media_box.x = media_arr->get(0)->val.number;
        page->media_box.y = media_arr->get(1)->val.number;
        page->media_box.width = media_arr->get(2)->val.number;
        page->media_box.height = media_arr->get(3)->val.number;
    }

    page->pdf = pdf;

    return page;
}

pdf_obj_t* _get_obj_from_table(pdf_file_t* pdf, pdf_indirect_t ref)
{
    if (pdf == NULL || ref.obj_num < 0)
        return NULL;
    int nums = pdf->read_objs.size();
    for (int i = 0; i < nums; i++)
    {
        if (pdf->read_objs[i]->indirect.obj_num == ref.obj_num 
            &&
            pdf->read_objs[i]->indirect.generation == ref.generation
        )
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
  
    pdf_dict* resources = NULL;
    if (obj->value->val.dict->has("/Resources"))
    {
        if (obj->value->val.dict->is_indirect("/Resources"))
        {
            pdf_indirect_t ref = obj->value->val.dict->get_indirect("/Resources");
            pdf_obj_t* res_obj = pdf_file_get_obj(obj->pdf, ref);
            if (res_obj == NULL || res_obj->value->type != PDF_VALUE_DICT)
                return;
            resources = res_obj->value->val.dict;
            }
            else if (obj->value->val.dict->is_dict("/Resources"))
            {
                resources = obj->value->val.dict->get_dict("/Resources");
            }
    }
    else
        return;

    obj->resources.extgstate_dict = NULL;
    if (resources->has("/ExtGState"))
    {
        if (resources->is_dict("/ExtGState"))
        {
            obj->resources.extgstate_dict = resources->get_dict("/ExtGState");
        }
        else if (resources->is_indirect("/ExtGState"))
        {
            pdf_indirect_t ext_ref = resources->get_indirect("/ExtGState");
            if (ext_ref.obj_num != -1)
            {
                pdf_obj_t* ext_obj = pdf_file_get_obj(obj->pdf, ext_ref);
                if (ext_obj != NULL)
                {
                    obj->resources.extgstate_dict = ext_obj->value->val.dict;
                }
            }
        }
    }

    obj->resources.colorspace_dict = NULL;
    if (resources->has("/ColorSpace"))
    {
        if (resources->is_dict("/ColorSpace"))
        {
            obj->resources.colorspace_dict = resources->get_dict("/ColorSpace");
        }
        else if (resources->is_indirect("/ColorSpace"))
        {
            pdf_indirect_t color_ref = resources->get_indirect("/ColorSpace");
            if (color_ref.obj_num != -1)
            {
                pdf_obj_t* color_obj = pdf_file_get_obj(obj->pdf, color_ref);
                if (color_obj != NULL)
                {
                    obj->resources.colorspace_dict = color_obj->value->val.dict;
                }
            }
        }
    }

    obj->resources.pattern_dict = NULL;
    if (resources->has("/Pattern"))
    {
        if (resources->is_dict("/Pattern"))
        {
            obj->resources.pattern_dict = resources->get_dict("/Pattern");
        }
        else if (resources->is_indirect("/Pattern"))
        {
            pdf_indirect_t pattern_ref = resources->get_indirect("/Pattern");
            if (pattern_ref.obj_num != -1)
            {
                pdf_obj_t* pattern_obj = pdf_file_get_obj(obj->pdf, pattern_ref);
                if (pattern_obj != NULL)
                {
                    obj->resources.pattern_dict = pattern_obj->value->val.dict;
                }
            }
        }
    }
  
    obj->resources.shading_dict = NULL;
    if (resources->has("/Shading"))
    {
        if (resources->is_dict("/Shading"))
        {
            obj->resources.shading_dict = resources->get_dict("/Shading");
        }
        else if (resources->is_indirect("/Shading"))
        {
            pdf_indirect_t shading_ref = resources->get_indirect("/Shading");
            if (shading_ref.obj_num != -1)
            {
                pdf_obj_t* shading_obj = pdf_file_get_obj(obj->pdf, shading_ref);
                if (shading_obj != NULL)
                {
                    obj->resources.shading_dict = shading_obj->value->val.dict;
                }
            }
        }
    }
  
    obj->resources.xobject_dict = NULL;
    if (resources->has("/XObject"))
    {
        if (resources->is_dict("/XObject"))
        {
            obj->resources.xobject_dict = resources->get_dict("/XObject");
        }
        else if (resources->is_indirect("/XObject"))
        {
            pdf_indirect_t xobj_ref = resources->get_indirect("/XObject");
            if (xobj_ref.obj_num != -1)
            {
                pdf_obj_t* xobj_obj = pdf_file_get_obj(obj->pdf, xobj_ref);
                if (xobj_obj != NULL)
                {
                    obj->resources.xobject_dict = xobj_obj->value->val.dict;
                }
            }
        }
    }
   
    obj->resources.font_dict = NULL;
    if (resources->has("/Font"))
    {
        if (resources->is_dict("/Font"))
        {
            obj->resources.font_dict = resources->get_dict("/Font");
        }
        else if (resources->is_indirect("/Font"))
        {
            pdf_indirect_t font_ref = resources->get_indirect("/Font");
            if (font_ref.obj_num != -1)
            {
                pdf_obj_t* font_obj = pdf_file_get_obj(obj->pdf, font_ref);
                if (font_obj != NULL)
                {
                    obj->resources.font_dict = font_obj->value->val.dict;
                }
            }
        }
    }
 
    obj->resources.procset_arr = NULL;
    if (resources->has("/ProcSet"))
    {
        if (resources->is_array("/ProcSet"))
        {
            obj->resources.procset_arr = resources->get_array("/ProcSet");
        }
        else if (resources->is_indirect("/ProcSet"))
        {
            pdf_indirect_t proc_ref = resources->get_indirect("/ProcSet");
            if (proc_ref.obj_num != -1)
            {
                pdf_obj_t* proc_obj = pdf_file_get_obj(obj->pdf, proc_ref);
                if (proc_obj != NULL)
                {
                    obj->resources.procset_arr = proc_obj->value->val.array;
                }
            }
        }
    }

    obj->resources.properties_dict = NULL;
    if (resources->has("/Properties"))
    {
        if (resources->is_dict("/Properties"))
        {
            obj->resources.properties_dict = resources->get_dict("/Properties");
        }
        else if (resources->is_indirect("/Properties"))
        {
            pdf_indirect_t prop_ref = resources->get_indirect("/Properties");
            if (prop_ref.obj_num != -1)
            {
                pdf_obj_t* prop_obj = pdf_file_get_obj(obj->pdf, prop_ref);
                if (prop_obj != NULL)
                {
                    obj->resources.properties_dict = prop_obj->value->val.dict;
                }
            }
        }
    }
}
pdf_obj_t* pdf_file_get_obj(pdf_file_t* pdf, pdf_indirect_t ref)
{
    if (pdf == NULL || ref.obj_num < 0)
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
        if (pdf->xref_table[i]->sequence == ref.obj_num
            &&
            pdf->xref_table[i]->generation == ref.generation
        )
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
                pdf_token* tk = pdf_parser_next_token(parser);
                if (tk == NULL || tk->type() != TOKEN_OBJ_BEG)
                {
                    delete tk;
                    pdf_parser_free(parser);
                    return NULL;
                }
                pdf_obj_t* obj = pdf_parser_build_obj(parser);
                if (obj == NULL)
                {
                    delete tk;
                    pdf_parser_free(parser);
                    return NULL;
                }
                obj->indirect.obj_num = ref.obj_num;
                obj->indirect.generation = ref.generation;

                obj->pdf = pdf;
                _fill_resources(obj);
                pdf->read_objs.push_back(obj);
                delete tk;
                pdf_parser_free(parser);
                return obj;
            }
            else
            {
                pdf_indirect_t obj_ref;
                obj_ref.obj_num = pdf->xref_table[i]->compressed.ref;
                obj_ref.generation = 0;
                pdf_obj_t* objs_obj = pdf_file_get_obj(pdf, obj_ref);
                if (objs_obj == NULL)
                {
                    return NULL;
                }

                int num_pairs = objs_obj->value->val.dict->get_number("/N");
                int first_offset = objs_obj->value->val.dict->get_number("/First");

                unsigned char* start = NULL;
                int size;
                pdf_stream_get_all(objs_obj->stream, &start, &size);
                if (start == NULL) return NULL;
                input_t* input = NULL;
                input_buffer(&input, (char*)start, size);
                unsigned char* origin = start;
                pdf_parser_t* parser = pdf_parser_init(pdf, input);
                pdf_token* tk = NULL;
                for (int j = 0; j < num_pairs; j++)
                {
                    tk = pdf_parser_next_token(parser);
                    int seq = atoi(tk->data());
                    delete tk;
                    tk = NULL;

                    tk = pdf_parser_next_token(parser);
                    int offset = atoi(tk->data());
                    delete tk;
                    tk = NULL;

                    unsigned char* p1 = start + first_offset + offset;
                    input_t* input2 = NULL;
                    input_buffer(&input2, (char*)p1, size - first_offset - offset);
                    pdf_parser_t* val_parser = pdf_parser_init(pdf, input2);

                    pdf_token* tk1 = pdf_parser_next_token(val_parser);
                    if (tk1 == NULL)
                    {
                        pdf_parser_free(val_parser);
                        return NULL;
                    }
                    pdf_obj_t* obj = pdf_obj_init();
                    obj->indirect.obj_num = seq;
                    obj->indirect.generation = 0;
                    
                    obj->value = (pdf_obj_value_t*)malloc(sizeof(pdf_obj_value_t));
                    if (tk1->type() == TOKEN_DICT_BEG)
                    {
                        pdf_dict* obj_dict = pdf_parser_build_dict(val_parser);
                        if (obj_dict == NULL)
                        {
                            pdf_parser_free(val_parser);
                            delete tk1;
                            return NULL;
                        }

                        obj->value->type = PDF_VALUE_DICT;
                        obj->value->val.dict = obj_dict;
                    }
                    else if (tk1->type() == TOKEN_ARRAY_BEG)
                    {
                        pdf_array* array = pdf_parser_build_array(val_parser);
                        if (array == NULL)
                        {
                            pdf_parser_free(val_parser);
                            delete tk1;
                            return NULL;
                        }
                        obj->value->type = PDF_VALUE_ARRAY;
                        obj->value->val.array = array;
                    }
                    else
                    {
                        pdf_parser_free(val_parser);
                        delete tk1;
                        return NULL;
                    }
                    delete tk1;
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
pdf_cmap* pdf_file_get_cmap(pdf_file_t* pdf, const char* name)
{
    if (pdf == NULL || name == NULL)
    {
        return NULL;
    }
    for (size_t i = 0 ; i < pdf->cmaps.size(); i++)
    {
        if (!strcmp(pdf->cmaps[i]->name, name))
        {
            return pdf->cmaps[i];
        }
    }

    FILE* f = NULL;
    char filename[256] = { 0 };
    sprintf(filename, "./CMaps/%s", name);
    f = fopen(filename, "rb");
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
    pdf_cmap* cmap = pdf_parser_build_cmap(parser);
    cmap->isGlobal = true;
    pdf_parser_free(parser);
    input_close(input);
    free(filedata);

    strcpy(cmap->name, name);
    pdf->cmaps.push_back(cmap);
    return cmap;
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

    for (size_t i = 0; i < file->cmaps.size(); i++)
    {
        delete file->cmaps[i];
    }

    if (file->input)
    {
        input_close(file->input);
        file->input = NULL;
    }
    if (file->trailer)
    {
        delete file->trailer;
        file->trailer = NULL;
    }
    delete file;
    file = NULL;
}