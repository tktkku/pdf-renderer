#include "pdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>

double get_wall_time(void)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (double)time.tv_sec + (double)time.tv_usec * 0.000001;
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

    double wall_start, wall_end;
    wall_start = get_wall_time();
    pdf_file_t* pdf = pdf_file_read_file(filename);
    int num_pages = pdf_file_get_pages(pdf);

    if (pages == NULL)
    {
        for (int i = 1; i <= num_pages; i++)
        {
            pdf_page_t* page = pdf_file_get_page(pdf, i - 1);
            if (page == NULL)
                continue;
            char filename[256] = { 0 };
            sprintf(filename, "page%d.png", i);
            render_to_png_by_plutovg(page, filename);

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
                render_to_png_by_plutovg(page, filename);

                pdf_page_free(page);
            }
        }

        free(pages);
    }

    pdf_file_free(pdf);
    wall_end = get_wall_time();
    printf("Elapsed %.3lf seconds.\n", wall_end - wall_start);

    return 0;
}