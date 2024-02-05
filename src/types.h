#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform_math.h"

#define UNUSED(x) (void)(x)

typedef struct {
	int32_t x, y;
} vec2i_t;

typedef struct rgba_t {
	uint8_t r, g, b, a;
} rgba_t;

typedef fix16_vec2_t vec2_t;

typedef fix16_vec3_t vec3_t;

typedef fix16_vec4_t vec4_t;

typedef fix16_mat44_t mat4_t;

typedef struct {
	vec3_t pos;
	vec2_t uv;
	rgba_t color;
} vertex_t;

typedef struct {
	vertex_t vertices[3];
} tris_t;

typedef struct {
	vertex_t vertices[4];
} quads_t;

typedef struct {
	vec3_t pos;
	rgb1555_t light;
} vertex_saturn_t;

typedef struct {
	vertex_saturn_t vertices[3];
} tris_saturn_t;

typedef struct {
	vertex_saturn_t vertices[4];
	rgb1555_t color;
	uint8_t useLight;
} quads_saturn_t;


typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t pixels[];
} character;


#define rgba(R, G, B, A) ((rgba_t){.r = R, .g = G, .b = B, .a = A})
#define vec2(X, Y) ((vec2_t){.x = FIX16(X), .y = FIX16(Y)})
#define vec2_fix16(X, Y) ((vec2_t){.x = X, .y = Y})
#define vec3(X, Y, Z) ((vec3_t){.x = FIX16(X), .y = FIX16(Y), .z = FIX16(Z)})
#define vec3_fix16(X, Y, Z) ((vec3_t){.x = X, .y = Y, .z = Z})
#define vec2i(X, Y) ((vec2i_t){.x = X, .y = Y})
#define vec4(X, Y, Z, W) ((vec4_t){.x = FIX16(X), .y = FIX16(Y), .z = FIX16(Z), .w = FIX16(W)})
#define vec4_fix16(X, Y, Z, W) ((vec4_t){.x = X, .y = Y, .z = Z, .w = W})

#define mat4(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) \
	(mat4_t){ \
		.row[0].x = m0,  .row[0].y = m1,  .row[0].z = m2,  .row[0].w = m3, \
		.row[1].x = m4,  .row[1].y = m5,  .row[1].z = m6,  .row[1].w = m7, \
		.row[2].x = m8,  .row[2].y = m9,  .row[2].z = m10, .row[2].w = m11, \
		.row[3].x = m12, .row[3].y = m13, .row[3].z = m14, .row[3].w = m15, \
	}

#define mat4_identity() \
	(mat4_t){   \
		.comp = { \
			.m00 = FIX16_ONE,  \
			.m01 = FIX16_ZERO, \
			.m02 = FIX16_ZERO, \
			.m03 = FIX16_ZERO, \
			.m10 = FIX16_ZERO, \
			.m11 = FIX16_ONE,  \
			.m12 = FIX16_ZERO, \
			.m13 = FIX16_ZERO, \
			.m20 = FIX16_ZERO, \
			.m21 = FIX16_ZERO, \
			.m22 = FIX16_ONE,  \
			.m23 = FIX16_ZERO, \
			.m30 = FIX16_ZERO, \
			.m31 = FIX16_ZERO, \
			.m32 = FIX16_ZERO, \
			.m33 = FIX16_ONE   \
		} \
	}


static inline vec2_t vec2_mulf(vec2_t a, fix16_t f) {
	return vec2_fix16(
		fix16_mul(a.x, f),
		fix16_mul(a.y, f)
	);
}

static inline vec2i_t vec2i_mulf(vec2i_t a, fix16_t f) {
	return vec2i(
		fix16_int32_to(fix16_int32_from(a.x) * f),
		fix16_int32_to(fix16_int32_from(a.y) * f)
	);
}


static inline vec3_t vec3_add(vec3_t a, vec3_t b) {
	return vec3_fix16(
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	);
}

static inline vec3_t vec3_sub(vec3_t a, vec3_t b) {
	return vec3_fix16(
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	);
}

static inline vec3_t vec3_mul(vec3_t a, vec3_t b) {
	return vec3_fix16(
		fix16_mul(a.x, b.x),
		fix16_mul(a.y, b.y),
		fix16_mul(a.z, b.z)
	);
}

static inline vec3_t vec3_mulf(vec3_t a, fix16_t f) {
	return vec3_fix16(
		fix16_mul(a.x, f),
		fix16_mul(a.y, f),
		fix16_mul(a.z, f)
	);
}

static inline vec3_t vec3_inv(vec3_t a) {
	return vec3_fix16(-a.x, -a.y, -a.z);
}

static inline vec3_t vec3_divf(vec3_t a, fix16_t f) {
	return vec3_fix16(
		fix16_div(a.x, f),
		fix16_div(a.y, f),
		fix16_div(a.z, f)
	);
}

static inline fix16_t vec3_len(vec3_t a) {
	return fix16_vec3_length(&a);
}

static inline vec3_t vec3_cross(vec3_t a, vec3_t b) {
	return vec3_fix16(
		fix16_mul(a.y, b.z) - fix16_mul(a.z, b.y),
		fix16_mul(a.z, b.x) - fix16_mul(a.x, b.z),
		fix16_mul(a.x, b.y) - fix16_mul(a.y, b.x)
	);
}

static inline fix16_t vec3_dot(vec3_t a, vec3_t b) {
	return fix16_vec3_dot(&a, &b);
}

static inline vec3_t vec3_lerp(vec3_t a, vec3_t b, fix16_t t) {
	return vec3_fix16(
		a.x + fix16_mul(t, (b.x - a.x)),
		a.y + fix16_mul(t, (b.y - a.y)),
		a.z + fix16_mul(t, (b.z - a.z))
	);
}

static inline vec3_t vec3_normalize(vec3_t a) {
	vec3_t res;
	fix16_vec3_normalized(&a, &res);
	return res;
}

static inline angle_t wrap_angle(angle_t a) {
	a = fmod(a + PLATFORM_PI, PLATFORM_2PI);
	if (a < FIX16_ZERO) {
		a += PLATFORM_PI;
	}
	return (a - PLATFORM_PI);
}

rgba_t rgba_from_u32(uint32_t v);
fix16_t vec3_angle_cos(vec3_t a, vec3_t b);
fix16_t vec3_angle(vec3_t a, vec3_t b);
vec3_t vec3_wrap_angle(vec3_t a);
vec3_t vec3_normalize(vec3_t a);
vec3_t vec3_project_to_ray(vec3_t p, vec3_t r0, vec3_t r1);
fix16_t vec3_distance_to_plane(vec3_t p, vec3_t plane_pos, vec3_t plane_normal);
vec3_t vec3_reflect(vec3_t incidence, vec3_t normal, fix16_t f);

angle_t wrap_angle(angle_t a);

vec3_t vec3_transform(vec3_t a, mat4_t *mat);
vec3_t vec3_rotate(vec3_t a, mat4_t *mat);
void mat4_set_translation(mat4_t *mat, vec3_t pos);
void mat4_set_yaw_pitch_roll(mat4_t *m, vec3_t rot);
void mat4_set_roll_pitch_yaw(mat4_t *mat, vec3_t rot);
void mat4_translate(mat4_t *mat, vec3_t translation);
void mat4_rot_inv(mat4_t *res, mat4_t *a);

void applyTransform(mat4_t *transf, mat4_t *res);
#endif
