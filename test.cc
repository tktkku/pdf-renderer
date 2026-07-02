#include "pdf-render.h"
#include "plutovg.h"
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
#include <thread>
#include <atomic>
#include <vector>
#include <stdint.h>
#include <time.h>
#include <chrono>
#include <locale.h>
#include "plutovg.h"
#include <memory>
#include "MiniFB.h"
#include "test.h"
static uint64_t get_wall_time(void)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}
std::vector<std::shared_ptr<unsigned char[]>> pages_pixel;
std::atomic<int> ready_pages = 0;

void render(pdf_file_t* pdf, 
    int window_width, int window_height, int window_stride, int dpi, int rotation, int i)
{
    pdf_page_t* page = pdf_file_get_page(pdf, i);
    if (page == NULL)
        return;

    pdf_render* r = pdf_render_init(page);
    pdf_render_build(r);
    std::shared_ptr<unsigned char[]> pixels(new unsigned char[window_height * window_stride]);
    memset(pixels.get(), 0xFF, window_height * window_stride);

    pdf_renderer_t* renderer = (pdf_renderer_t*)malloc(sizeof(pdf_renderer_t));
    renderer->vtable = &plutovg_vtable;

    int rotationDegree = 0;
    if (rotation == 0) {
        bool isCanvasLandscape = window_width > window_height;
        bool isPageLandscape = pdf_page_get_media_width(page) > pdf_page_get_media_height(page);
        rotationDegree = (isCanvasLandscape != isPageLandscape) ? 90 : 0;
    }
    else {
        rotationDegree = 90 * (rotation - 1);
    }

    int renderWidth = window_width;
    int renderHeight = window_height;
    int renderStride = window_stride;

    plutovg_surface_t* surface =
        plutovg_surface_create_for_data(pixels.get(),
            renderWidth, renderHeight, renderStride);
    plutovg_canvas_t* canvas = plutovg_canvas_create(surface);
    plutovg_canvas_save(canvas);
    plutovg_canvas_set_rgb(canvas, 1, 1, 1);
    plutovg_canvas_paint(canvas);
    plutovg_canvas_set_rgb(canvas, 0, 0, 0);
    plutovg_canvas_set_line_width(canvas, 0.5);
    plutovg_canvas_set_miter_limit(canvas, 10.0);

    plutovg_canvas_restore(canvas);

    double pageWidth = pdf_page_get_media_width(page);
    double pageHeight = pdf_page_get_media_height(page);
    // convert canvas' px size to pt size
    double canvasWidthPt = renderWidth * 72.0 / dpi;
    double canvasHeightPt = renderHeight * 72.0 / dpi;
    // calc scale factor
    double scaleX = canvasWidthPt / (rotationDegree % 180 == 90 ? pageHeight : pageWidth);
    double scaleY = canvasHeightPt / (rotationDegree % 180 == 90 ? pageWidth : pageHeight);
    double scale = fmin(scaleX, scaleY); // fit to page
    // after scale, the actual content size
    double scaledWidth = pageWidth * scale;
    double scaledHeight = pageHeight * scale;
    // calc offset (in pt)
    double offsetX = (canvasWidthPt - scaledWidth) / 2.0;
    double offsetY = (canvasHeightPt - scaledHeight) / 2.0;
    // px -> pt -> rotate -> offset -> scale
    plutovg_canvas_translate(canvas, 0, renderHeight); // flip y in px
    plutovg_canvas_scale(canvas, dpi / 72.0, -(dpi / 72.0)); // px to pt
    // move to center then rotate
    plutovg_canvas_translate(canvas, canvasWidthPt / 2.0, canvasHeightPt / 2.0);
    plutovg_canvas_rotate(canvas, rotationDegree * 3.14159 / 180.0);
    plutovg_canvas_translate(canvas, -canvasWidthPt / 2.0, -canvasHeightPt / 2.0);

    // move and scale
    plutovg_canvas_translate(canvas, offsetX, offsetY);
    plutovg_canvas_scale(canvas, scale, scale);

    renderer->user_data = canvas;

    pdf_render_run(r, renderer);

    // pages_pixel.push_back(std::move(pixels));
    // int p = ready_pages.load(std::memory_order_acquire);
    // ready_pages.store(p + 1, std::memory_order_release);
    char name[128];
    snprintf(name, sizeof(name), "page-%d.png", i);
    plutovg_surface_write_to_png(surface, name);

    free(renderer);
    plutovg_canvas_destroy(canvas);
    plutovg_surface_destroy(surface);

    pdf_render_free(r);
    pdf_page_free(page);
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
    
    int window_width = 1678, window_height = 2373;
    int dpi = 203;
    int window_stride = window_width * 4;
    int rotation = 1;
  
    std::thread t([&] {
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
        if (pdf == NULL)
            return -1;

        int num_pages = pdf_file_get_pages(pdf);
        if (pages == NULL)
        {
            for (int i = 1; i <= num_pages; i++)
            {
                render(pdf, window_width, window_height, window_stride, dpi, rotation, i - 1);
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
                    render(pdf, window_width, window_height, window_stride, dpi, rotation, i - 1);
                }
            }

            free(pages);
        }

        pdf_file_free(pdf);
        free(filebuffer);

        wall_end = get_wall_time();
        printf("Elapsed %ld ms.\n", wall_end - wall_start);
    });



    // struct mfb_window* window = mfb_open_ex("Pdf Viewer", window_width, window_height, WF_RESIZABLE);
    // int cur_index = 0;
    // mfb_set_target_fps(1);
    // mfb_show_cursor(window, true);
    // mfb_update_state state;
    // mfb_set_mouse_button_callback(
    //         [](struct mfb_window* window, mfb_mouse_button button, 
    //             mfb_key_mod mod, bool is_pressed) mutable {
            
    //     }, window);
    // mfb_set_keyboard_callback([&cur_index, &state]
    //     (struct mfb_window* window, mfb_key key, mfb_key_mod mod, bool is_pressed) mutable {
    //         if (is_pressed)
    //         {
    //             if (key == KB_KEY_A)
    //             {
    //                 if (cur_index > 0) cur_index--;
    //             }
    //             else if (key == KB_KEY_D)
    //             {
    //                 if (cur_index < ready_pages.load(std::memory_order_acquire)) cur_index++;
    //             }
    //         }
    //         if (cur_index < ready_pages.load(std::memory_order_acquire))
    //         {
    //             state = mfb_update(window, pages_pixel[cur_index].get());
    //         }
    // }, window);
    // int cur_x = -1, cur_y = -1;
    // mfb_set_mouse_move_callback([&cur_x, &cur_y](struct mfb_window* window, int x, int y) mutable {
    //     cur_x = x;
    //     cur_y = y;
        
    // }, window);
    // mfb_set_mouse_scroll_callback([](struct mfb_window* window, mfb_key_mod mod, float delta_x, float delta_y) mutable {
        
    // }, window);
    // do {
    //     if (cur_index >= 0 && cur_index < ready_pages.load(std::memory_order_acquire))
    //     {
    //         auto p = pages_pixel[cur_index];
    //         state = mfb_update(window, p.get());
    //         if (state != STATE_OK)
    //             break;
    //     }
    // } while (mfb_wait_sync(window));

    t.join();

    return 0;
}