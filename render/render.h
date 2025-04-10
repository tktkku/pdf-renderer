#pragma once
#include "pdf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_MODULE_H 
#include <plutovg.h>
#include "plutovg-stb-image-write.h"
#include "plutovg-stb-image.h"

typedef struct pdf_stack_node
{
    char* data;
    size_t size;
    struct pdf_stack_node* next;
} pdf_stack_node_t;

typedef struct pdf_stack
{
    pdf_stack_node_t* top;
} pdf_stack_t;

void pdf_value_free(struct pdf_value* value);

pdf_stack_t* pdf_stack_init(void);
void pdf_stack_push(pdf_stack_t* s, const void* data, size_t size);
void pdf_stack_pop(pdf_stack_t* s, pdf_stack_node_t* data);
void pdf_stack_free(pdf_stack_t* s);
void pdf_stack_show(pdf_stack_t* s);

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
        FT_Face ft_face;
        bool font_face_loaded;
        pdf_font_t* font;
        double textLineWidth;
    } textState;
    struct pdf_graphics_state* next;
} pdf_graphics_state_t;
typedef struct
{
    pdf_font_t* font;
    FT_Face ft_face;
    plutovg_font_face_t* fontface;
    bool loaded;
} pdf_font_cache_t;
typedef struct context
{
    FT_Library ft_library;
    plutovg_surface_t* surface;
    pdf_stack_t* stack;
    plutovg_canvas_t* canvas;
    pdf_file_t* pdf;
    pdf_page_t* page;
    pdf_obj_t* current_obj;
    pdf_graphics_state_t* state;
    cvector_vector_type(pdf_font_cache_t*) fontcache;
} pdf_context_t;
typedef void (*OPERATION_HANDLER)(pdf_context_t* context);

typedef struct {
    const char* operation;
    OPERATION_HANDLER handler;
} handler_entry;

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

const static handler_entry handlers[] = {
    {"\"", handle_quotation}, {"'", handle_apostrophe}, {"B", handle_B},
    {"B*", handle_B_star},    {"BDC", handle_BDC},      {"BMC", handle_BMC},
    {"BI", handle_BI},        {"BT", handle_BT},        {"CS", handle_CS},
    {"DP", handle_DP},        {"Do", handle_Do},        {"EI", handle_EI},
    {"EMC", handle_EMC},      {"ET", handle_ET},        {"F", handle_F_f},
    {"G", handle_G},          {"ID", handle_ID},        {"J", handle_J},
    {"K", handle_K},          {"M", handle_M},          {"MP", handle_MP},
    {"Q", handle_Q},          {"RG", handle_RG},        {"S", handle_S},
    {"SC", handle_SC},        {"SCN", handle_SCN},      {"T*", handle_T_star},
    {"TD", handle_TD},        {"TJ", handle_TJ},        {"TL", handle_TL},
    {"Tc", handle_Tc},        {"Td", handle_Td},        {"Tf", handle_Tf},
    {"Tj", handle_Tj},        {"Tm", handle_Tm},        {"Tr", handle_Tr},
    {"Ts", handle_Ts},        {"Tw", handle_Tw},        {"Tz", handle_Tz},
    {"W", handle_W},          {"W*", handle_W_star},    {"b", handle_b},
    {"b*", handle_b_star},    {"c", handle_c},          {"cm", handle_cm},
    {"cs", handle_cs},        {"d", handle_d},          {"d0", handle_d0},
    {"d1", handle_d1},        {"f", handle_F_f},        {"f*", handle_f_star},
    {"g", handle_g},          {"gs", handle_gs},        {"h", handle_h},
    {"i", handle_i},          {"j", handle_j},          {"k", handle_k},
    {"l", handle_l},          {"m", handle_m},          {"n", handle_n},
    {"q", handle_q},          {"re", handle_re},        {"rg", handle_rg},
    {"ri", handle_ri},        {"s", handle_s},          {"sc", handle_sc},
    {"scn", handle_scn},      {"sh", handle_sh},        {"v", handle_v},
    {"w", handle_w},          {"y", handle_y} };
void _do_render_operation(pdf_context_t* context, pdf_parser_token_t* tk);
void stroke(pdf_context_t* context);
void _do_text_render(pdf_context_t* context, char* buf, int len);