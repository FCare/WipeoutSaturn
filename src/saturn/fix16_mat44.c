#include <string.h>

#include "fixed_math.h"

void
fix16_mat44_dup(const fix16_mat44_t *m0, fix16_mat44_t *result)
{
    const fix16_t *arr_ptr;
    arr_ptr = m0->arr;

    fix16_t *result_arr_ptr;
    result_arr_ptr = result->arr;

    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;

    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;

    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;

    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr++ = *arr_ptr++;
    *result_arr_ptr   = *arr_ptr;
}

void
fix16_mat44_identity(fix16_mat44_t *m0)
{
    fix16_t *arr_ptr;
    arr_ptr = m0->arr;

    *arr_ptr++ = FIX16_ONE;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ONE;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ONE;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr   = FIX16_ONE;
}

void
fix16_mat44_mul(const fix16_mat44_t *m0, const fix16_mat44_t *m1, fix16_mat44_t *result)
{
    fix16_vec4_t transposed_row;

    const fix16_vec4_t * const m00 = (const fix16_vec4_t *)&m0->row[0];
    const fix16_vec4_t * const m01 = (const fix16_vec4_t *)&m0->row[1];
    const fix16_vec4_t * const m02 = (const fix16_vec4_t *)&m0->row[2];
    const fix16_vec4_t * const m03 = (const fix16_vec4_t *)&m0->row[3];

    mat44_row_transpose(&m1->arr[0], &transposed_row);
    result->frow[0][0] = fix16_vec4_dot(m00, &transposed_row);
    result->frow[1][0] = fix16_vec4_dot(m01, &transposed_row);
    result->frow[2][0] = fix16_vec4_dot(m02, &transposed_row);
    result->frow[3][0] = fix16_vec4_dot(m03, &transposed_row);

    mat44_row_transpose(&m1->arr[1], &transposed_row);
    result->frow[0][1] = fix16_vec4_dot(m00, &transposed_row);
    result->frow[1][1] = fix16_vec4_dot(m01, &transposed_row);
    result->frow[2][1] = fix16_vec4_dot(m02, &transposed_row);
    result->frow[3][1] = fix16_vec4_dot(m03, &transposed_row);

    mat44_row_transpose(&m1->arr[2], &transposed_row);
    result->frow[0][2] = fix16_vec4_dot(m00, &transposed_row);
    result->frow[1][2] = fix16_vec4_dot(m01, &transposed_row);
    result->frow[2][2] = fix16_vec4_dot(m02, &transposed_row);
    result->frow[3][2] = fix16_vec4_dot(m03, &transposed_row);

    mat44_row_transpose(&m1->arr[3], &transposed_row);
    result->frow[0][3] = fix16_vec4_dot(m00, &transposed_row);
    result->frow[1][3] = fix16_vec4_dot(m01, &transposed_row);
    result->frow[2][3] = fix16_vec4_dot(m02, &transposed_row);
    result->frow[3][3] = fix16_vec4_dot(m03, &transposed_row);
}

void
fix16_mat44_x_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle)
{
    fix16_t sin;
    fix16_t cos;

    fix16_sincos(angle, &sin, &cos);

    const fix16_t m01 = m0->frow[0][1];
    const fix16_t m02 = m0->frow[0][2];
    const fix16_t m11 = m0->frow[1][1];
    const fix16_t m12 = m0->frow[1][2];
    const fix16_t m21 = m0->frow[2][1];
    const fix16_t m22 = m0->frow[2][2];

    result->frow[0][1] =  fix16_mul(m01, cos) + fix16_mul(m02, sin);
    result->frow[0][2] = -fix16_mul(m01, sin) + fix16_mul(m02, cos);
    result->frow[1][1] =  fix16_mul(m11, cos) + fix16_mul(m12, sin);
    result->frow[1][2] = -fix16_mul(m11, sin) + fix16_mul(m12, cos);
    result->frow[2][1] =  fix16_mul(m21, cos) + fix16_mul(m22, sin);
    result->frow[2][2] = -fix16_mul(m21, sin) + fix16_mul(m22, cos);
}

void
fix16_mat44_y_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle)
{
    fix16_t sin_value;
    fix16_t cos_value;

    fix16_sincos(angle, &sin_value, &cos_value);

    const fix16_t m00 = m0->frow[0][0];
    const fix16_t m02 = m0->frow[0][2];
    const fix16_t m10 = m0->frow[1][0];
    const fix16_t m12 = m0->frow[1][2];
    const fix16_t m20 = m0->frow[2][0];
    const fix16_t m22 = m0->frow[2][2];

    result->frow[0][0] = fix16_mul(m00, cos_value) - fix16_mul(m02, sin_value);
    result->frow[0][2] = fix16_mul(m00, sin_value) + fix16_mul(m02, cos_value);
    result->frow[1][0] = fix16_mul(m10, cos_value) - fix16_mul(m12, sin_value);
    result->frow[1][2] = fix16_mul(m10, sin_value) + fix16_mul(m12, cos_value);
    result->frow[2][0] = fix16_mul(m20, cos_value) - fix16_mul(m22, sin_value);
    result->frow[2][2] = fix16_mul(m20, sin_value) + fix16_mul(m22, cos_value);
}

void
fix16_mat44_z_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle)
{
    fix16_t sin_value;
    fix16_t cos_value;

    fix16_sincos(angle, &sin_value, &cos_value);

    const fix16_t m00 = m0->frow[0][0];
    const fix16_t m01 = m0->frow[0][1];
    const fix16_t m10 = m0->frow[1][0];
    const fix16_t m11 = m0->frow[1][1];
    const fix16_t m20 = m0->frow[2][0];
    const fix16_t m21 = m0->frow[2][1];

    result->frow[0][0] =  fix16_mul(m00, cos_value) + fix16_mul(m01, sin_value);
    result->frow[0][1] = -fix16_mul(m00, sin_value) + fix16_mul(m01, cos_value);
    result->frow[1][0] =  fix16_mul(m10, cos_value) + fix16_mul(m11, sin_value);
    result->frow[1][1] = -fix16_mul(m10, sin_value) + fix16_mul(m11, cos_value);
    result->frow[2][0] =  fix16_mul(m20, cos_value) + fix16_mul(m21, sin_value);
    result->frow[2][1] = -fix16_mul(m20, sin_value) + fix16_mul(m21, cos_value);
}

void
fix16_mat44_rotation_create(fix16_mat44_t *m0, angle_t rx, angle_t ry, angle_t rz)
{
    fix16_t sx;
    fix16_t cx;

    fix16_sincos(rx, &sx, &cx);

    fix16_t sy;
    fix16_t cy;

    fix16_sincos(ry, &sy, &cy);

    fix16_t sz;
    fix16_t cz;

    fix16_sincos(rz, &sz, &cz);

    const fix16_t sxsy = fix16_mul(sx, sy);
    const fix16_t cxsy = fix16_mul(cx, sy);

    m0->frow[0][0] = fix16_mul(   cy, cz);
    m0->frow[0][1] = fix16_mul( sxsy, cz) + fix16_mul(cx, sz);
    m0->frow[0][2] = fix16_mul(-cxsy, cz) + fix16_mul(sx, sz);
    m0->frow[1][0] = fix16_mul(  -cy, sz);
    m0->frow[1][1] = fix16_mul(-sxsy, sz) + fix16_mul(cx, cz);
    m0->frow[1][2] = fix16_mul( cxsy, sz) + fix16_mul(sx, cz);
    m0->frow[2][0] = sy;
    m0->frow[2][1] = fix16_mul(  -sx, cy);
    m0->frow[2][2] = fix16_mul(   cx, cy);
}

void
fix16_mat44_rotation_set(const fix16_mat33_t *r, fix16_mat44_t *result)
{
    const fix16_t *r_arr_ptr;
    r_arr_ptr = r->arr;

    fix16_t *result_arr_ptr;
    result_arr_ptr = result->arr;

    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr++ = *r_arr_ptr++;
    result_arr_ptr++;

    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr++ = *r_arr_ptr++;
    result_arr_ptr++;

    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr++ = *r_arr_ptr++;
    *result_arr_ptr   = *r_arr_ptr;
}

size_t
fix16_mat44_str(const fix16_mat44_t *m0, char *buffer, int32_t decimals)
{
    char *buffer_ptr;
    buffer_ptr = buffer;

    for (uint32_t i = 0; i < 3; i++) {
        *buffer_ptr++ = '|';
        buffer_ptr += fix16_vec4_str(&m0->row[i], buffer_ptr, decimals);
        *buffer_ptr++ = '|';
        *buffer_ptr++ = '\n';
    }
    *buffer_ptr = '\0';

    return (buffer_ptr - buffer);
}

void
fix16_mat44_zero(fix16_mat44_t *m0)
{
    fix16_t *arr_ptr;
    arr_ptr = m0->arr;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;

    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr++ = FIX16_ZERO;
    *arr_ptr   = FIX16_ZERO;
}

void
mat44_row_transpose(const fix16_t *arr, fix16_vec4_t *m0)
{
    m0->x = arr[0];
    m0->y = arr[4];
    m0->z = arr[8];
    m0->w = arr[12];
}
