#pragma once
#include "pdf-private.h"
#include "pdf-render.h"
#include <cmath>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <plutovg.h>
#include <vector>
#include <memory>
#include <functional>
#include "plutovg-stb-truetype.h"
typedef struct {
    stbtt_vertex* vertices;
    int nvertices;
    int index;
    int advance_width;
    int left_side_bearing;
    int x1;
    int y1;
    int x2;
    int y2;
} glyph_t;
#define GLYPH_CACHE_SIZE 256
typedef void (*pdf_destroy_func_t)(void* closure);
typedef struct {
    int ref_count;
    int ascent;
    int descent;
    int line_gap;
    int x1;
    int y1;
    int x2;
    int y2;
    stbtt_fontinfo info;
    glyph_t** glyphs[GLYPH_CACHE_SIZE];
    pdf_destroy_func_t destroy_func;
    void* closure;
} pdf_font_face_t;
void pdf_font_face_destroy(pdf_font_face_t* face);
typedef struct pdf_cff_char_render
{
    bool fisr_stack_clear;
    unsigned char* buf;
    unsigned char* cur;
    int len;
    pdf_array* charstrings;
    pdf_array* global_subr;
    uint16_t global_bias;
    plutovg_canvas_t* canvas;
    double fontSize;
    double curX;
    double curY;
    double width;
    int stems;
    int stemshm;
    bool open;
    bool havewidth;
    double transient[32];
} pdf_cff_char_render_t;
typedef struct {
    plutovg_matrix_t textMatrix;
    plutovg_matrix_t textLineMatrix;
    double characterSpacing;
    double wordSpacing;
    double horizontalScaling;
    double textLeading;
    double fontSize;
    int textMode;
    double textRise;
    double TJValue;
    pdf_font_face_t* fontface;
    bool font_face_loaded;
    pdf_font_t* font;
    double textLineWidth;
 } pdf_text_state_t;
typedef struct pdf_graphics_state {
    struct {
        char currentColorSpace[256];
        double color[3];
    } fill;
    struct {
        char currentColorSpace[256];
        double color[3];
    } stroke;
    double lineWidth;
    int lineCap;
    int lineJoin;
    double miterLimit;
    struct {
        double* dashs;
        int dash_size;
        double offset;
    } dashPattern;
    pdf_text_state_t textState;
    struct pdf_graphics_state* next;
} pdf_graphics_state_t;
typedef struct
{
    pdf_font_t* font;
    pdf_font_face_t* fontface;
    bool loaded;
} pdf_font_cache_t;
struct pdf_render_command
{
    pdf_token_type_t type;
    union 
    {
        struct {
            char color[64];
        } cs, CS;
        struct {
            float g;
        } g, G;
        struct {
            float r, g, b;
        } k, K, rg, RG, sc, SC;
        struct {
            float x1, y1, x2, y2, x3, y3;
        } c, v, y, Tm, cm;
        struct {
            float x, y;
        } l, m, Td, TD;
        struct {
            float x, y, width, height;
        } re;
        struct {
            float f;
        } w, Tc, TL,Ts, Tw, Tz, M;
        struct {
            int i;
        } Tr, j, J;
        struct {
            std::unique_ptr<pdf_node> data;
        } apostrophe, Tj;
        struct {
            std::shared_ptr<pdf_deque> tmp_deque;
        } TJ;
        struct {
            pdf_font_t* font;
            pdf_font_face_t* fontface;
            float size;
        } Tf;
        struct {
            float offset;
            float dashs[2];
        } d;
        struct {
            xobject_type_t type;
            pdf_xobject_t* xobj;
            int width, height;
            plutovg_surface_t* surface;
            std::vector<std::unique_ptr<pdf_render_command>> opts;
        } Do;
    };
    pdf_render_command()
    {

    }
    ~pdf_render_command() 
    {
        
    }
};
struct pdf_render
{
    unsigned char* pixels;
    int width;
    int height;
    int stride;
    plutovg_surface_t* surface;
    pdf_deque* deque;
    plutovg_canvas_t* canvas;
    pdf_file_t* pdf;
    pdf_page_t* page;
    pdf_obj_t* current_obj;
    pdf_graphics_state_t* state;
    std::vector<pdf_font_cache_t*> fontcache;
    std::vector<std::unique_ptr<pdf_render_command>> operations;
};
typedef void (*OPERATION_HANDLER)(pdf_render* context, pdf_render_command* cmd, bool dry_run);

void handle_q(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Q(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_cm(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_w(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_J(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_j(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_M(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_d(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_ri(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_i(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_gs(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_m(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_l(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_c(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_v(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_y(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_h(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_re(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_S(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_s(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_F_f(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_f_star(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_B(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_B_star(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_b(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_b_star(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_n(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_W(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_W_star(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_CS(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_SC(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_G(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_cs(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_sc(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_g(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_RG(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_rg(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_K(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_k(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_SCN(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_scn(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_sh(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Do(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_BI(pdf_render* context, pdf_render_command* cmd, bool dry_run);
//void handle_ID(pdf_render* context, pdf_render_command* cmd, bool dry_run);
//void handle_EI(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_BT(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_ET(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tf(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tc(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tw(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tz(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_TL(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tr(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Ts(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Td(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_TD(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tm(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_T_star(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_Tj(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_apostrophe(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_quotation(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_TJ(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_d0(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_d1(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_BDC(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_BMC(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_DP(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_EMC(pdf_render* context, pdf_render_command* cmd, bool dry_run);
void handle_MP(pdf_render* context, pdf_render_command* cmd, bool dry_run);

const static OPERATION_HANDLER handlers[] = {
    NULL, handle_quotation, handle_apostrophe, handle_B, 
    handle_B_star, handle_BDC, handle_BMC, handle_BI, 
    handle_BT, handle_CS, handle_DP, handle_Do, NULL,
    handle_EMC, handle_ET, handle_F_f, handle_G, NULL, 
    handle_J, handle_K, handle_M, handle_MP, handle_Q, handle_RG, 
    handle_S, handle_SC, handle_SCN, handle_T_star, handle_TD, 
    handle_TJ, handle_TL,handle_Tc, handle_Td, handle_Tf, handle_Tj, 
    handle_Tm, handle_Tr, handle_Ts, handle_Tw, handle_Tz, handle_W, 
    handle_W_star, handle_b, handle_b_star, handle_c, handle_cm, 
    handle_cs, handle_d, handle_d0, handle_d1, handle_F_f, handle_f_star,
    handle_g, handle_gs, handle_h, handle_i, handle_j, handle_k, handle_l, 
    handle_m, handle_n, handle_q, handle_re, handle_rg, handle_ri, handle_s, 
    handle_sc, handle_scn, handle_sh, handle_v, handle_w, handle_y
};
pdf_render_command* _do_render_operation(pdf_stream_t* stream, pdf_render* context, pdf_token* tk);
void stroke(pdf_render* context);
void _do_text_render(pdf_render* context, char* buf, int len);
void _init_state(pdf_render* context);

typedef void (*CFF_HANDLER)(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hstem(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vstem(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vmoveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_rlineto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hlineto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vlineto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_rrcurveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_callsubr(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hstemhm(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hintmask(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_cntrmask(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_rmoveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hmoveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vstemhm(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_rcurveline(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_rlinecurve(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vvcurveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hhcurveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_callgsubr(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_vhcurveto(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hvcurveto(pdf_cff_char_render_t* context, pdf_deque* deque);

const static CFF_HANDLER CFF_HANDLERS1[] = {
    NULL,
    handle_hstem,
    NULL,
    handle_vstem,
    handle_vmoveto,
    handle_rlineto,
    handle_hlineto,
    handle_vlineto,
    handle_rrcurveto,
    NULL,
    handle_callsubr,//10
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    handle_hstemhm,
    handle_hintmask,
    handle_cntrmask,//20
    handle_rmoveto,
    handle_hmoveto,
    handle_vstemhm,
    handle_rcurveline,
    handle_rlinecurve,
    handle_vvcurveto,
    handle_hhcurveto,
    NULL,
    handle_callgsubr,
    handle_vhcurveto,//30
    handle_hvcurveto,
};
void handle_and(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_or(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_not(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_abs(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_add(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_sub(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_div(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_neg(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_eq(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_drop(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_put(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_get(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_ifelse(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_random(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_mul(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_sqrt(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_dup(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_exch(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_index(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_roll(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hflex(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_flex(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_hflex1(pdf_cff_char_render_t* context, pdf_deque* deque);
void handle_flex1(pdf_cff_char_render_t* context, pdf_deque* deque);

const static CFF_HANDLER CFF_HANDLERS2[] = {
    NULL, NULL, NULL,
    handle_and,
    handle_or,
    handle_not,
    NULL, NULL, NULL, 
    handle_abs,
    handle_add,//10
    handle_sub,
    handle_div,
    NULL,
    handle_neg,
    handle_eq,
    NULL, NULL,
    handle_drop,
    NULL,
    handle_put, //20
    handle_get,
    handle_ifelse,
    handle_random,
    handle_mul,
    NULL,
    handle_sqrt,
    handle_dup,
    handle_exch,
    handle_index,
    handle_roll,//30
    NULL, NULL, NULL,
    handle_hflex,
    handle_flex,
    handle_hflex1,
    handle_flex1
};
void _cff_do_render_char(pdf_cff_char_render_t* context, pdf_deque* deque);

#define PDF_OPERATION_PATH_FILL     0x01
#define PDF_OPERATION_PATH_STROKE   0x02
#define PDF_OPERATION_PATH_NON_ZERO 0x04
#define PDF_OPERATION_PATH_EVEN_ODD 0x08
#define PDF_OPERATION_PATH_CLOSE    0x10
#define PDF_OPERATION_PATH_CLIP     0x20
#define PDF_OPERATION_PATH_NEW_PATH 0x40