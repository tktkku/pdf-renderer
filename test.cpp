#include "pdf.h"
#include "pdf-render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define strtok_r strtok_s
#else
#include <unistd.h>
#include <sys/time.h>
#include <getopt.h>
#endif

#include <stdint.h>
#include <time.h>
#include <chrono>
#include <locale.h>
#include "plutovg.h"
static uint64_t get_wall_time(void)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

int main(int argc, char* argv[])
{
    const char* filename = NULL;
    char* pages = NULL;
    for (int i = 1; i < argc; )
    {
        if (!strcmp(argv[i], "-f"))
        {
            i++;
            filename = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "-p"))
        {
            i++;
            pages = strdup(argv[i]);
            i++;
        }
        else
        {
            i++;
        }
    }
    if (filename == NULL)
    {
        filename = argv[1];
    }
    //setbuf(stdout, NULL);
    setlocale(LC_CTYPE, "zh_CN.UTF-8");

    uint64_t wall_start, wall_end;
    wall_start = get_wall_time();
    // pdf_file_t* pdf = pdf_file_read_file(filename);
    FILE* f = fopen(filename, "rb");
    if (f == NULL)
        return -1;
    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* filebuffer = (char*)malloc(filesize);
    fread(filebuffer, 1, filesize, f);
    fclose(f);
    pdf_file_t* pdf = pdf_file_read_buffer(filebuffer, filesize);
    int num_pages = pdf_file_get_pages(pdf);

    f = fopen("fonts/SimSun.ttf", "rb");
    fseek(f, 0, SEEK_END);
    filesize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* fontbuffer = (char*)malloc(filesize);
    if (fontbuffer == nullptr)
    {
        pdf_file_free(pdf);
        fclose(f);
        return -1;
    }
    fread(fontbuffer, 1, filesize, f);
    fclose(f);

    pdf_file_load_font(pdf, "SimSun", fontbuffer, filesize);
    if (pages == NULL)
    {
        for (int i = 1; i <= num_pages; i++)
        {
            pdf_page_t* page = pdf_file_get_page(pdf, i - 1);
            if (page == NULL)
                continue;
            char filename[256] = { 0 };
            sprintf(filename, "page%d.png", i);
            int height = (int)(pdf_page_get_media_height(page) * PIXELS_PER_POINT + 0.5);
            int width = (int)(pdf_page_get_media_width(page) * PIXELS_PER_POINT + 0.5);
            int stride = width * 4;
            pdf_render_t* r = pdf_render_init_with_size(page, width, height, stride, 203);
            pdf_render_do(r);
            unsigned char* pixels = (unsigned char*)malloc(static_cast<size_t>(stride) * height);
            if (pixels == nullptr)
            {
                pdf_page_free(page);
                break;
            }
            memset(pixels, 0xFF, static_cast<size_t>(stride) * height);
            pdf_render_copy_to_buffer(r, pixels, stride * height);
            plutovg_surface_t* surface =
            plutovg_surface_create_for_data(pixels, width, height, stride);
            plutovg_surface_write_to_png(surface, filename);
            plutovg_surface_destroy(surface);
            free(pixels);
            pdf_render_free(r);
            pdf_page_free(page);
        }
    }
    else
    {
        char* token;
        char* rest = pages;
        
        while ((token = strtok_r(rest, ",", &rest)) != NULL)
        {
            char* dash = strchr(token, '-');
            int start = 0;
            int end = 0;
            if (dash)
            {
                *dash = '\0';
                start = atoi(token);
                end = atoi(dash + 1);
            }
            else
            {
                start = atoi(token);
                end = start;
            }

            if (start - 1 < 0) start = 1; 
            if (start > num_pages) start = num_pages;
            if (end - 1 < 0) end = 1; 
            if (end > num_pages) end = num_pages;
            for (int i = start; i <= end; i++)
            {
                pdf_page_t* page = pdf_file_get_page(pdf, i - 1);
                if (page == NULL)
                    continue;
                char filename[256] = { 0 };
                sprintf(filename, "page%d.png", i);
                pdf_render_t* r = pdf_render_init(page, 203);
                pdf_render_do(r);
                pdf_render_save_to_png(r, filename);

                pdf_page_free(page);
            }
        }

        free(pages);
    }

    pdf_file_free(pdf);
    free(filebuffer);
    free(fontbuffer);
    wall_end = get_wall_time();
    printf("Elapsed %ld ms.\n", wall_end - wall_start);

    return 0;
}