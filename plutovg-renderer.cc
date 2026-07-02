#include "pdf-render.h"
#include <cstddef>
#include <plutovg.h>
#include <plutovg-private.h>
#include <math.h>
#include <typeinfo>
#include <algorithm>
#include "test.h"
#include "plutovg-stb-image-write.h"
#include "plutovg-stb-image.h"
#include "plutovg/include/plutovg.h"

static void set_color(pdf_renderer_t* renderer, float r, float g, float b)
{
    plutovg_color_t color;
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = 1.0f;
    plutovg_canvas_set_color((plutovg_canvas_t*)renderer->user_data, &color);
}
static void get_color(pdf_renderer_t* renderer, float *r, float *g, float *b)
{
    plutovg_canvas_t* canvas = (plutovg_canvas_t*)renderer->user_data;
    *r = canvas->state->color.r;
    *g = canvas->state->color.g;
    *b = canvas->state->color.b;
}
static void stroke(pdf_renderer_t* renderer)
{
    plutovg_canvas_stroke((plutovg_canvas_t*)renderer->user_data);
}
static void fill(pdf_renderer_t* renderer)
{
    plutovg_canvas_fill((plutovg_canvas_t*)renderer->user_data);
}
static void set_fill_rule(pdf_renderer_t* renderer, bool even_odd)
{
    plutovg_canvas_set_fill_rule((plutovg_canvas_t*)renderer->user_data, 
        even_odd ? PLUTOVG_FILL_RULE_EVEN_ODD : PLUTOVG_FILL_RULE_NON_ZERO);
}
static void close_path(pdf_renderer_t* renderer)
{
    plutovg_canvas_close_path((plutovg_canvas_t*)renderer->user_data);
}
static void new_path(pdf_renderer_t* renderer)
{
    plutovg_canvas_new_path((plutovg_canvas_t*)renderer->user_data);
}
static void cubic_to(pdf_renderer_t* renderer, float x1, float y1, float x2, float y2, float x3, float y3)
{
    plutovg_canvas_cubic_to((plutovg_canvas_t*)renderer->user_data, x1, y1, x2, y2, x3, y3);
}
static void line_to(pdf_renderer_t* renderer, float x, float y)
{
    plutovg_canvas_line_to((plutovg_canvas_t*)renderer->user_data, x, y);
}
static void move_to(pdf_renderer_t* renderer, float x, float y)
{
    plutovg_canvas_move_to((plutovg_canvas_t*)renderer->user_data, x, y);
}
static void rect(pdf_renderer_t* renderer, float x, float y, float width, float height)
{
    plutovg_canvas_rect((plutovg_canvas_t*)renderer->user_data, x, y, width, height);
}
static void get_current_point(pdf_renderer_t* renderer, float *x, float *y)
{
    plutovg_canvas_get_current_point((plutovg_canvas_t*)renderer->user_data, x, y);
}
static void set_line_width(pdf_renderer_t* renderer, float width)
{
    plutovg_canvas_set_line_width((plutovg_canvas_t*)renderer->user_data, width);
}
static void transform(pdf_renderer_t* renderer, float a, float b, float c, float d, float e, float f)
{
    plutovg_matrix_t ctm;
    plutovg_matrix_init(&ctm, 
        a, b, c, d, e, f);
    plutovg_canvas_transform((plutovg_canvas_t*)renderer->user_data, &ctm);
}
static void set_dash_offset(pdf_renderer_t* renderer, float offset)
{
    plutovg_canvas_set_dash_offset((plutovg_canvas_t*)renderer->user_data, offset);
}
static void set_dash_array(pdf_renderer_t* renderer, float* dashs, int nums)
{
    plutovg_canvas_set_dash_array((plutovg_canvas_t*)renderer->user_data, dashs, nums);
}
static void set_line_join(pdf_renderer_t* renderer, int join)
{
    plutovg_canvas_set_line_join((plutovg_canvas_t*)renderer->user_data, (plutovg_line_join_t)join);
}
static void set_line_cap(pdf_renderer_t* renderer, int cap)
{
    plutovg_canvas_set_line_cap((plutovg_canvas_t*)renderer->user_data, (plutovg_line_cap_t)cap);
}
static void set_miter_limit(pdf_renderer_t* renderer, float limit)
{
    plutovg_canvas_set_miter_limit((plutovg_canvas_t*)renderer->user_data, limit);
}
static void clip(pdf_renderer_t* renderer)
{
    plutovg_canvas_clip((plutovg_canvas_t*)renderer->user_data);
}
static void restore(pdf_renderer_t* renderer)
{
    plutovg_canvas_restore((plutovg_canvas_t*)renderer->user_data);
}
static void save(pdf_renderer_t* renderer)
{
    plutovg_canvas_save((plutovg_canvas_t*)renderer->user_data);
}
static void scale(pdf_renderer_t* renderer, float sx, float sy)
{
    plutovg_canvas_scale((plutovg_canvas_t*)renderer->user_data, sx, sy);
}
static void draw_image(pdf_renderer_t* renderer, int width, int height, int channels, unsigned char* pixles, int size)
{
    if (width <= 0 || height <= 0 || channels <= 0 || pixles == NULL || size <= 0)
    {
        return;
    }
    plutovg_surface_t* s = NULL;
    if (memcmp(pixles, "\xFF\xD8", 2) == 0) // jpeg
    {
        s = plutovg_surface_load_from_image_data(pixles,
            size);
        // unsigned char* pixels = stbi_load_from_memory(xobj->image->data,
        // xobj->image->data_len, &width, &height, &channels, channels);
        // //convert_to_gray(pixels, width, height, width * channels, channels);
        // //average_gray(pixels, width, height, width * channels, channels);
        // //dither_by_threshold(pixels, width, height, width * channels,
        // channels, 220); free(xobj->image->data); xobj->image->data = pixels;
        // xobj->image->data_len = width * channels * height;
        // dither_by_threshold(pixels, width, height, width * channels, 220);
        // stbi_image_free(pixels);
    }
    else if (memcmp(pixles, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",
        8) == 0) // png
    {
        s = plutovg_surface_load_from_image_data(pixles,
            size);
    }
    else if (memcmp(pixles, "P6", 2) == 0) // ppm
    {
        s = plutovg_surface_load_from_image_data(pixles,
            size);
    }
    else
    {
        // ponytail: skip PPM roundtrip, write raw pixels directly to RGBA surface
        s = plutovg_surface_create(width, height);
        if (s)
        {
            int surface_stride = plutovg_surface_get_stride(s);
            unsigned char* surface_data = plutovg_surface_get_data(s);
            int src_line_bytes = size / height;
            if (channels == 3)
            {
                for (int i = 0; i < height; i++)
                {
                    unsigned char* src = pixles + i * src_line_bytes;
                    unsigned char* dst = surface_data + i * surface_stride;
                    for (int j = 0; j < width; j++)
                    {
                        dst[j*4+0] = src[j*3+0];
                        dst[j*4+1] = src[j*3+1];
                        dst[j*4+2] = src[j*3+2];
                        dst[j*4+3] = 255;
                    }
                }
            }
            else // channels == 1
            {
                for (int i = 0; i < height; i++)
                {
                    unsigned char* src = pixles + i * src_line_bytes;
                    unsigned char* dst = surface_data + i * surface_stride;
                    for (int j = 0; j < width; j++)
                    {
                        unsigned char v = src[j];
                        dst[j*4+0] = v;
                        dst[j*4+1] = v;
                        dst[j*4+2] = v;
                        dst[j*4+3] = 255;
                    }
                }
            }
        }
    }
    if (channels == 3)
    {
        // convert_to_gray(xobj->image->data, width, height, width * channels,
        // channels); otsu(xobj->image->data, width, height, width * channels,
        // channels);
    }

    if (s == NULL)
    {
        return;
    }
    
    plutovg_canvas_save((plutovg_canvas_t*)renderer->user_data);
    float scale_x = 1.f / width;
    float scale_y = 1.f / height;
    plutovg_matrix_t m = { scale_x,  0,
                            0, -scale_y,
                            0, height * scale_y };
    plutovg_canvas_set_texture((plutovg_canvas_t*)renderer->user_data, s, PLUTOVG_TEXTURE_TYPE_PLAIN,
        1.0f, &m);
    plutovg_canvas_paint((plutovg_canvas_t*)renderer->user_data);
    plutovg_canvas_restore((plutovg_canvas_t*)renderer->user_data);
}
const pdf_renderer_vtable_t plutovg_vtable = {
    set_color,
    get_color,

    set_fill_rule,
    set_dash_offset,
    set_dash_array,
    set_line_join,
    set_line_cap,
    set_miter_limit,
    set_line_width,
    get_current_point,
    scale,
    stroke,
    fill,
    close_path,
    new_path,
    cubic_to,
    line_to,
    move_to,
    rect,
    clip,
    transform,

    save,
    restore,
   
    draw_image,
};