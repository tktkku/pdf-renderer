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
#include <vector>
#include <stdint.h>
#include <time.h>
#include <chrono>
#include <locale.h>
#include "plutovg.h"

#include "MiniFB.h"

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
    int window_width = 900, window_height = 600;
    int window_stride = window_width * 4;
    int window_size = window_stride * window_height;
    std::vector<unsigned char*> page_pixels;
    if (pages == NULL)
    {
        for (int i = 1; i <= num_pages; i++)
        {
            pdf_page_t* page = pdf_file_get_page(pdf, i - 1);
            if (page == NULL)
                continue;
            
            pdf_render* r = pdf_render_init_for_paper(page, window_width, window_height, window_stride, 1, 203);
            pdf_render_do(r);
            unsigned char* pixels = (unsigned char*)malloc(window_size);
            if (pixels == nullptr)
            {
                pdf_page_free(page);
                break;
            }
            memset(pixels, 0xFF, window_size);
            pdf_render_copy_to_buffer(r, pixels, window_size);
            page_pixels.push_back(pixels);
            // free(pixels);
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
                
                pdf_render* r = pdf_render_init_for_paper(page, window_width, window_height, window_stride, 1, 203);
                pdf_render_do(r);
                unsigned char* pixels = (unsigned char*)malloc(window_size);
                if (pixels == nullptr)
                {
                    pdf_page_free(page);
                    break;
                }
                memset(pixels, 0xFF, window_size);
                pdf_render_copy_to_buffer(r, pixels, window_size);
                page_pixels.push_back(pixels);
                // free(pixels);
                pdf_render_free(r);
                pdf_page_free(page);
            }
        }

        free(pages);
    }

    pdf_file_free(pdf);
    free(filebuffer);

    wall_end = get_wall_time();
    printf("Elapsed %ld ms.\n", wall_end - wall_start);

    struct mfb_window* window = mfb_open_ex("Pdf Viewer", window_width, window_height, WF_RESIZABLE);
    int cur_index = 0;
    mfb_set_target_fps(1);
    mfb_show_cursor(window, true);
    mfb_update_state state;
    mfb_set_mouse_button_callback(
            [&cur_index, &state, page_pixels](struct mfb_window* window, mfb_mouse_button button, 
                mfb_key_mod mod, bool is_pressed) mutable {
            
            if (is_pressed)
            {
                if (button == MOUSE_LEFT)
                {
                    if (cur_index > 0) cur_index--;
                }
                else if (button == MOUSE_RIGHT)
                {
                    if (cur_index < page_pixels.size() - 1) cur_index++;
                }
            }
            state = mfb_update(window, page_pixels[cur_index]);
        }, window);
    int cur_x = -1, cur_y = -1;
    mfb_set_mouse_move_callback([&cur_x, &cur_y](struct mfb_window* window, int x, int y) mutable {
        cur_x = x;
        cur_y = y;
        
    }, window);
    mfb_set_mouse_scroll_callback([](struct mfb_window* window, mfb_key_mod mod, float delta_x, float delta_y) mutable {
        
    }, window);
    do {
        state = mfb_update(window, page_pixels[cur_index]);
        if (state != STATE_OK)
            break;
    } while (mfb_wait_sync(window));

    return 0;
}