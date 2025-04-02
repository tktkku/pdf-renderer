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
    if (argc < 2)
    {
        exit(EXIT_FAILURE);
    }
    //setbuf(stdout, NULL);
    setlocale(LC_CTYPE, "zh_CN.UTF-8");
 
    double wall_start, wall_end;
    wall_start = get_wall_time();
    pdf_file_t* pdf = pdf_file_read_file(argv[1]);
    int num_pages = pdf_file_get_pages(pdf);
    for (int i = 0; i < num_pages; i++)
    {
        pdf_page_t* page = pdf_file_get_page(pdf, i);
        if (page == NULL)
            continue;
        char filename[256] = {0};
        sprintf(filename, "page%d.png", i);
        render_to_png_by_plutovg(page, filename);

        pdf_page_free(page);
    }

    pdf_file_free(pdf);
    wall_end = get_wall_time();
    printf("Elapsed %.3lf seconds.\n", wall_end - wall_start);

    return 0;
}