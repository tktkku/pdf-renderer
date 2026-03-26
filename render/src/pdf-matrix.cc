#include "pdf-render-private.h"
void pdf_matrix_init(pdf_matrix_t* matrix, float a, float b, float c, float d, float e, float f)
{
    matrix->a = a; matrix->b = b;
    matrix->c = c; matrix->d = d;
    matrix->e = e; matrix->f = f;
}

void pdf_matrix_multiply(pdf_matrix_t* matrix, const pdf_matrix_t* left, const pdf_matrix_t* right)
{
    float a = left->a * right->a + left->b * right->c;
    float b = left->a * right->b + left->b * right->d;
    float c = left->c * right->a + left->d * right->c;
    float d = left->c * right->b + left->d * right->d;
    float e = left->e * right->a + left->f * right->c + right->e;
    float f = left->e * right->b + left->f * right->d + right->f;
    pdf_matrix_init(matrix, a, b, c, d, e, f);
}
void pdf_matrix_init_translate(pdf_matrix_t* matrix, float tx, float ty)
{
    pdf_matrix_init(matrix, 1, 0, 0, 1, tx, ty);
}
void pdf_matrix_init_identity(pdf_matrix_t* matrix)
{
    pdf_matrix_init(matrix, 1, 0, 0, 1, 0, 0);
}
void pdf_matrix_init_scale(pdf_matrix_t* matrix, float sx, float sy)
{
    pdf_matrix_init(matrix, sx, 0, 0, sy, 0, 0);
}
void pdf_matrix_translate(pdf_matrix_t* matrix, float tx, float ty)
{
    pdf_matrix_t m;
    pdf_matrix_init_translate(&m, tx, ty);
    pdf_matrix_multiply(matrix, &m, matrix);
}
void pdf_matrix_scale(pdf_matrix_t* matrix, float sx, float sy)
{
    pdf_matrix_t m;
    pdf_matrix_init_scale(&m, sx, sy);
    pdf_matrix_multiply(matrix, &m, matrix);
}

void pdf_matrix_map(const pdf_matrix_t* matrix, float x, float y, float* xx, float* yy)
{
    *xx = x * matrix->a + y * matrix->c + matrix->e;
    *yy = x * matrix->b + y * matrix->d + matrix->f;
}

void pdf_matrix_map_point(const pdf_matrix_t* matrix, const pdf_point_t* src, pdf_point_t* dst)
{
    pdf_matrix_map(matrix, src->x, src->y, &dst->x, &dst->y);
}

void pdf_matrix_map_points(const pdf_matrix_t* matrix, const pdf_point_t* src, pdf_point_t* dst, int count)
{
    for(int i = 0; i < count; ++i) {
        pdf_matrix_map_point(matrix, &src[i], &dst[i]);
    }
}
