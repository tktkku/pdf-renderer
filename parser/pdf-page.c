#include "pdf-private.h"
#include "pdf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include <assert.h>
#include <stdbool.h>

pdf_page_t* pdf_page_init()
{
    pdf_page_t* page = (pdf_page_t*)malloc(sizeof(pdf_page_t));
    if (page == NULL)
    {
        return NULL;
    }
    memset(page, 0, sizeof(pdf_page_t));

    return page;
}

int pdf_page_get_streams(pdf_page_t* page)
{
    if (page == NULL) return 0;

    if (page->contents == NULL) return 0;

    return page->num_contents;
}
// void pdf_page_get_ext_gstate(pdf_page_t* page, const char* name)
// {
//     if (page == NULL)
//         return;
//     if (page->resources == NULL)
//         return;
//     if (page->resources->ext_gstate == NULL)
//         return;
//     pdf_dict_t* ext_gstate = pdf_dict_get_dict(page->resources->ext_gstate, name);
//     if (ext_gstate == NULL)
//     {
//         int ref = pdf_dict_get_ref(page->resources->ext_gstate, name);
//         if (ref == -1)
//         {
//             return;
//         }
//         pdf_obj_t* obj = pdf_file_get_obj(page->pdf, ref);
//         ext_gstate = obj->value->val.dict;
//         if (ext_gstate == NULL)
//         {
//             return;
//         }
//     }
//     //const char* Type = pdf_dict_get_name(ext_gstate, "/Type");// shall be ExtGState
//     //double LW = pdf_dict_get_number(ext_gstate, "/LW"); // line width
//     //int LC = pdf_dict_get_number(ext_gstate, "/LC"); // line cap
//     //int LJ = pdf_dict_get_number(ext_gstate, "/LJ"); // line join
//     //double ML = pdf_dict_get_number(ext_gstate, "/ML"); // miter limit
//     //pdf_array_t* D = pdf_dict_get_array(ext_gstate, "/D"); // dash pattern
//     //const char* RI = pdf_dict_get_name(ext_gstate, "/RI");
//     //int OP = pdf_dict_get_bool(ext_gstate, "/OP"); // whether to apply overprint
//     //int op = pdf_dict_get_bool(ext_gstate, "/op");
//     //int OPM = pdf_dict_get_number(ext_gstate, "/OPM");
//     //pdf_array_t* Font = pdf_dict_get_array(ext_gstate, "/Font");
//     // void* BG;
//     // void* BG2;
//     // void* UCR;
//     // void* UCR2;
//     // void* TR;
//     // void* TR2;
//     // void* HT;
//     // double FL = pdf_dict_get_number(ext_gstate, "/FL");
//     // double SM = pdf_dict_get_number(ext_gstate, "/SM");
//     // int SA = pdf_dict_get_bool(ext_gstate, "/SA");
//     // void* BM;
//     // void* SMask;
//     // double CA = pdf_dict_get_number(ext_gstate, "/CA");
//     // double ca = pdf_dict_get_number(ext_gstate, "/ca");
//     // int AIS = pdf_dict_get_bool(ext_gstate, "/AIS");
//     // int TK = pdf_dict_get_bool(ext_gstate, "/TK");
// }
uint32_t _str_to_32bit(char* str, int len)
{
    if (str == NULL || len <= 0 || len > 8) return 0;
    uint32_t ret = 0;
    for (int i = 0; i < len; i++)
    {
        ret = (ret << 8) | str[i];
    }
    return ret;
}
uint32_t _hex_str_to_32bit(char* hexStr, int len)
{
    if (hexStr == NULL || len <= 0 || (len != 2 && len != 4 && len != 8)) return 0;
    // convert every 8 bit to hex value
    uint8_t tmp[8] = {0};
    for (int i = 0; i < len; i++)
    {
        if (hexStr[i] >= '0' && hexStr[i] <= '9')
        {
            tmp[i] = hexStr[i] - '0';
        }
        else if (hexStr[i] >= 'A' && hexStr[i] <= 'F')
        {
            tmp[i] = hexStr[i] - 'A' + 10;
        }
        else if (hexStr[i] >= 'a' && hexStr[i] <= 'f')
        {
            tmp[i] = hexStr[i] - 'a' + 10;
        }
    }
    uint32_t ret = 0;
    for (int i = 0; i < len; i++)
    {
        ret = (ret << 4) | tmp[i];
    }
    
    return ret;
}

uint16_t _hex_str_to_16bit(char hexStr[4])
{
    // convert every 8 bit to hex value
    uint16_t tmp[4];
    for (int i = 0; i < 4; i++)
    {
        if (hexStr[i] >= '0' && hexStr[i] <= '9')
        {
            tmp[i] = hexStr[i] - '0';
        }
        else if (hexStr[i] >= 'A' && hexStr[i] <= 'F')
        {
            tmp[i] = hexStr[i] - 'A' + 10;
        }
        else if (hexStr[i] >= 'a' && hexStr[i] <= 'f')
        {
            tmp[i] = hexStr[i] - 'a' + 10;
        }
    }
    return (
        ((tmp[0] << 12) & 0xF000)
        | ((tmp[1] << 8) & 0x0F00)
        | ((tmp[2] << 4) & 0x00F0)
        | ((tmp[3]) & 0x000F)
        );
}

uint8_t _hex_str_to_8bit(char hexStr[2])
{
    uint8_t tmp[2];
    for (int i = 0; i < 2; i++)
    {
        if (hexStr[i] >= '0' && hexStr[i] <= '9')
        {
            tmp[i] = hexStr[i] - '0';
        }
        else if (hexStr[i] >= 'A' && hexStr[i] <= 'F')
        {
            tmp[i] = hexStr[i] - 'A' + 10;
        }
        else if (hexStr[i] >= 'a' && hexStr[i] <= 'f')
        {
            tmp[i] = hexStr[i] - 'a' + 10;
        }
    }
    return (
        ((tmp[0] << 4) & 0xF0)
        |
        ((tmp[1]) & 0x0F)
        );
}

pdf_stream_t* pdf_page_get_stream(pdf_page_t* page, int index)
{
    if (page == NULL) return NULL;
    if (page->contents == NULL) return NULL;
    if (index > page->num_contents - 1) return NULL;

    pdf_obj_t* obj = page->contents[index];
    if (obj == NULL)
    {
        return false;
    }

    return obj->stream;
}

void pdf_page_free(pdf_page_t* page)
{
    if (page == NULL) return;

    // if (page->resources)
    // {
    //     free(page->resources);
    //     page->resources = NULL;
    // }

    if (page->contents)
    {
        free(page->contents);
        page->contents = NULL;
    }

    free(page);
    page = NULL;
}

int pdf_page_get_media_width(pdf_page_t* page)
{
    if (page == NULL) return -1;
    return page->media_box.width;
}
int pdf_page_get_media_height(pdf_page_t* page)
{
    if (page == NULL) return -1;
    return page->media_box.height;
}