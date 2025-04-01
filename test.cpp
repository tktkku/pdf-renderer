#include "pdf.h"

#include <stdio.h>
#include <locale.h>
#include <chrono>
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        exit(EXIT_FAILURE);
    }
    //setbuf(stdout, NULL);
    setlocale(LC_CTYPE, "zh_CN.UTF-8");
 
    auto start = std::chrono::high_resolution_clock::now();
    pdf_file_t* pdf = pdf_file_read_file(argv[1]);
    int num_pages = pdf_file_get_pages(pdf);
    for (int i = 0; i < num_pages; i++)
    {
        PdfPage* page = pdf_file_get_page(pdf, i);
        if (page == NULL)
            continue;
        char filename[256] = {0};
        sprintf(filename, "page%d.png", i);
        render_to_png_by_plutovg(page, filename);

        pdf_file_free_page(pdf, page);
    }

    pdf_file_free(pdf);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double time_sec = duration.count() * 1e-9;
    printf("Elapsed %.3lf seconds.\n", time_sec);

    return 0;
}