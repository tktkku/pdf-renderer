#include "parser/pdf.h"
#include "pdf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>
#include <locale.h>

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        exit(EXIT_FAILURE);
    }
    //setbuf(stdout, NULL);
    setlocale(LC_CTYPE, "zh_CN.UTF-8");
 
    clock_t start, finish;
    start = clock();
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
    finish = clock();
    double duration = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("Elapsed %.3lf seconds.\n", duration);

    return 0;
}