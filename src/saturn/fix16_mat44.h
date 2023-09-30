#ifndef _YAUL_GAMEMATH_FIX16_H_
#error "Header file must not be directly included"
#endif /* !_YAUL_GAMEMATH_FIX16_H_ */

/// @addtogroup MATH_FIX16_MATRIX
/// @defgroup MATH_FIX16_MATRIX4X3 4x3
/// @ingroup MATH_FIX16_MATRIX
/// @{

/// @brief Not yet documented.
#define FIX16_MAT44_COLUMNS   (4)

/// @brief Not yet documented.
#define FIX16_MAT44_ROWS      (4)

/// @brief Not yet documented.
#define FIX16_MAT44_ARR_COUNT (FIX16_MAT44_COLUMNS * FIX16_MAT44_ROWS)

/// @cond
union fix16_vec4;
union fix16_mat33;

typedef union fix16_vec4 fix16_vec4_t;
typedef union fix16_mat33 fix16_mat33_t;
/// @endcond

/// @brief Not yet documented.
///
/// @note Row-major matrix.
typedef union fix16_mat44 {
    /// @brief Not yet documented.
    struct {
        /// @brief Not yet documented. Row 0.
        fix16_t m00, m01, m02, m03;
        /// @brief Not yet documented. Row 1.
        fix16_t m10, m11, m12, m13;
        /// @brief Not yet documented. Row 2.
        fix16_t m20, m21, m22, m23;
        /// @brief Not yet documented. Row 3.
        fix16_t m30, m31, m32, m33;
    } comp;

    /// @brief Not yet documented.
    fix16_t arr[FIX16_MAT44_ARR_COUNT];
    /// @brief Not yet documented.
    fix16_t frow[FIX16_MAT44_ROWS][FIX16_MAT44_COLUMNS];
    /// @brief Not yet documented.
    fix16_vec4_t row[FIX16_MAT44_ROWS];
} __aligned(4) fix16_mat44_t;

/// @brief Not yet documented.
///
/// @param[out] m0 Not yet documented.
/// @param      t  Not yet documented.
static inline void __always_inline
fix16_mat44_translation_set(fix16_mat44_t *m0, const fix16_vec4_t *t)
{
    m0->frow[0][3] = t->x;
    m0->frow[1][3] = t->y;
    m0->frow[2][3] = t->z;
}

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      x      Not yet documented.
static inline void __always_inline
fix16_mat44_x_translate(const fix16_mat44_t *m0, fix16_mat44_t *result, fix16_t x)
{
    result->frow[0][3] = m0->frow[0][3] + x;
}

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      y      Not yet documented.
static inline void __always_inline
fix16_mat44_y_translate(const fix16_mat44_t *m0, fix16_mat44_t *result, fix16_t y)
{
    result->frow[1][3] = m0->frow[1][3] + y;
}

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      z      Not yet documented.
static inline void __always_inline
fix16_mat44_z_translate(const fix16_mat44_t *m0, fix16_mat44_t *result, fix16_t z)
{
    result->frow[2][3] = m0->frow[2][3] + z;
}

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      t      Not yet documented.
static inline void __always_inline
fix16_mat44_translate(const fix16_mat44_t *m0, fix16_mat44_t *result, const fix16_vec4_t *t)
{
    fix16_mat44_x_translate(m0, result, t->x);
    fix16_mat44_y_translate(m0, result, t->y);
    fix16_mat44_z_translate(m0, result, t->z);
}

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
extern void fix16_mat44_dup(const fix16_mat44_t *m0, fix16_mat44_t *result);

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param      m1     Not yet documented.
/// @param[out] result Not yet documented.
extern void fix16_mat44_mul(const fix16_mat44_t *m0, const fix16_mat44_t *m1, fix16_mat44_t *result);

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      angle  Not yet documented.
extern void fix16_mat44_x_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle);

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      angle  Not yet documented.
extern void fix16_mat44_y_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle);

/// @brief Not yet documented.
///
/// @param      m0     Not yet documented.
/// @param[out] result Not yet documented.
/// @param      angle  Not yet documented.
extern void fix16_mat44_z_rotate(const fix16_mat44_t *m0, fix16_mat44_t *result, angle_t angle);

/// @brief Not yet documented.
///
/// @param[out] m0 Not yet documented.
/// @param      rx Not yet documented.
/// @param      ry Not yet documented.
/// @param      rz Not yet documented.
extern void fix16_mat44_rotation_create(fix16_mat44_t *m0, angle_t rx, angle_t ry, angle_t rz);

/// @brief Not yet documented.
///
/// @param      r      Not yet documented.
/// @param[out] result Not yet documented.
extern void fix16_mat44_rotation_set(const fix16_mat33_t *r, fix16_mat44_t *result);

/// @brief Not yet documented.
///
/// @param m0 Not yet documented.
extern void fix16_mat44_identity(fix16_mat44_t *m0);

/// @brief Not yet documented.
///
/// @param      m0       Not yet documented.
/// @param[out] buffer   Not yet documented.
/// @param      decimals Not yet documunted.
///
/// @returns The string length, not counting the `NUL` character.
extern size_t fix16_mat44_str(const fix16_mat44_t *m0, char *buffer, int32_t decimals);

/// @brief Not yet documented.
///
/// @param m0 Not yet documented.
extern void fix16_mat44_zero(fix16_mat44_t *m0);

extern void mat44_row_transpose(const fix16_t *arr, fix16_vec4_t *m0);

/// @}
