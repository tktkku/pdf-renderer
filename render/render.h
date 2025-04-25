#pragma once
#include "pdf.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <plutovg.h>
#include "plutovg-stb-image-write.h"
#include "plutovg-stb-image.h"

typedef struct pdf_cff_char_render
{
    bool fisr_stack_clear;
    unsigned char* buf;
    unsigned char* cur;
    int len;
    pdf_array_t* charstrings;
    pdf_array_t* global_subr;
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
    struct {
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
        plutovg_font_face_t* fontface;
        bool font_face_loaded;
        pdf_font_t* font;
        double textLineWidth;
    } textState;
    struct pdf_graphics_state* next;
} pdf_graphics_state_t;
typedef struct
{
    pdf_font_t* font;
    plutovg_font_face_t* fontface;
    bool loaded;
} pdf_font_cache_t;
typedef struct context
{
    plutovg_surface_t* surface;
    pdf_deque_t* deque;
    plutovg_canvas_t* canvas;
    pdf_file_t* pdf;
    pdf_page_t* page;
    pdf_obj_t* current_obj;
    pdf_graphics_state_t* state;
    cvector_vector_type(pdf_font_cache_t*) fontcache;
} pdf_context_t;
typedef void (*OPERATION_HANDLER)(pdf_context_t* context);

void handle_q(pdf_context_t* context);
void handle_Q(pdf_context_t* context);
void handle_cm(pdf_context_t* context);
void handle_w(pdf_context_t* context);
void handle_J(pdf_context_t* context);
void handle_j(pdf_context_t* context);
void handle_M(pdf_context_t* context);
void handle_d(pdf_context_t* context);
void handle_ri(pdf_context_t* context);
void handle_i(pdf_context_t* context);
void handle_gs(pdf_context_t* context);
void handle_m(pdf_context_t* context);
void handle_l(pdf_context_t* context);
void handle_c(pdf_context_t* context);
void handle_v(pdf_context_t* context);
void handle_y(pdf_context_t* context);
void handle_h(pdf_context_t* context);
void handle_re(pdf_context_t* context);
void handle_S(pdf_context_t* context);
void handle_s(pdf_context_t* context);
void handle_F_f(pdf_context_t* context);
void handle_f_star(pdf_context_t* context);
void handle_B(pdf_context_t* context);
void handle_B_star(pdf_context_t* context);
void handle_b(pdf_context_t* context);
void handle_b_star(pdf_context_t* context);
void handle_n(pdf_context_t* context);
void handle_W(pdf_context_t* context);
void handle_W_star(pdf_context_t* context);
void handle_CS(pdf_context_t* context);
void handle_SC(pdf_context_t* context);
void handle_G(pdf_context_t* context);
void handle_cs(pdf_context_t* context);
void handle_sc(pdf_context_t* context);
void handle_g(pdf_context_t* context);
void handle_RG(pdf_context_t* context);
void handle_rg(pdf_context_t* context);
void handle_K(pdf_context_t* context);
void handle_k(pdf_context_t* context);
void handle_SCN(pdf_context_t* context);
void handle_scn(pdf_context_t* context);
void handle_sh(pdf_context_t* context);
void handle_Do(pdf_context_t* context);
void handle_BI(pdf_context_t* context);
void handle_ID(pdf_context_t* context);
void handle_EI(pdf_context_t* context);
void handle_BT(pdf_context_t* context);
void handle_ET(pdf_context_t* context);
void handle_Tf(pdf_context_t* context);
void handle_Tc(pdf_context_t* context);
void handle_Tw(pdf_context_t* context);
void handle_Tz(pdf_context_t* context);
void handle_TL(pdf_context_t* context);
void handle_Tr(pdf_context_t* context);
void handle_Ts(pdf_context_t* context);
void handle_Td(pdf_context_t* context);
void handle_TD(pdf_context_t* context);
void handle_Tm(pdf_context_t* context);
void handle_T_star(pdf_context_t* context);
void handle_Tj(pdf_context_t* context);
void handle_apostrophe(pdf_context_t* context);
void handle_quotation(pdf_context_t* context);
void handle_TJ(pdf_context_t* context);
void handle_d0(pdf_context_t* context);
void handle_d1(pdf_context_t* context);
void handle_BDC(pdf_context_t* context);
void handle_BMC(pdf_context_t* context);
void handle_DP(pdf_context_t* context);
void handle_EMC(pdf_context_t* context);
void handle_MP(pdf_context_t* context);

const static OPERATION_HANDLER handlers[] = {
    NULL, handle_quotation, handle_apostrophe, handle_B, 
    handle_B_star, handle_BDC, handle_BMC, handle_BI, 
    handle_BT, handle_CS, handle_DP, handle_Do, handle_EI,
    handle_EMC, handle_ET, handle_F_f, handle_G, handle_ID, 
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
void _do_render_operation(pdf_context_t* context, pdf_parser_token_t* tk);
void stroke(pdf_context_t* context);
void _do_text_render(pdf_context_t* context, char* buf, int len);

typedef void (*CFF_HANDLER)(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hstem(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vstem(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_rlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vlineto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_rrcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_callsubr(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hstemhm(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hintmask(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_cntrmask(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_rmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hmoveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vstemhm(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_rcurveline(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_rlinecurve(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vvcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hhcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_callgsubr(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_vhcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hvcurveto(pdf_cff_char_render_t* context, pdf_deque_t* deque);

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
void handle_and(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_or(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_not(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_abs(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_add(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_sub(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_div(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_neg(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_eq(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_drop(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_put(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_get(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_ifelse(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_random(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_mul(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_sqrt(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_dup(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_exch(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_index(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_roll(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hflex(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_flex(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_hflex1(pdf_cff_char_render_t* context, pdf_deque_t* deque);
void handle_flex1(pdf_cff_char_render_t* context, pdf_deque_t* deque);

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