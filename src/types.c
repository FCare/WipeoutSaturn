#include "platform_math.h"
#include "types.h"
#include "utils.h"

rgba_t rgba_from_u32(uint32_t v) {
	return rgba(
		((v >> 24) & 0xff),
		((v >> 16) & 0xff),
		((v >> 8) & 0xff),
		255
	);
}

vec3_t vec3_wrap_angle(vec3_t a) {
	return vec3_fix16(wrap_angle(a.x), wrap_angle(a.y), wrap_angle(a.z));
}

fix16_t vec3_angle(vec3_t a, vec3_t b) {
	fix16_t magnitude = fix16_sqrt(fix16_mul(fix16_vec3_sqr_length(&a), fix16_vec3_sqr_length(&b)));
	fix16_t cosine = (magnitude == FIX16_ZERO)
		? FIX16_ONE
		: fix16_div(vec3_dot(a, b), magnitude);
	return acos(clamp(cosine, -1, 1));
}

static vec4_t transpose(const fix16_vec4_t *vec) {
	static fix16_vec4_t res;
	mat44_row_transpose(&vec->comp[0], &res);
	return vec4_fix16(res.x, res.y,res.z, res.w);
}

vec3_t vec3_transform(vec3_t a, mat4_t *mat) {
	fix16_t w = fix16_mul(mat->arr[3], a.x) + fix16_mul(mat->arr[7], a.y) + fix16_mul(mat->arr[11], a.z) + mat->arr[15];
 	if (w == FIX16_ZERO) {
   	w = FIX16_ONE;
	}
	return vec3_fix16(
         fix16_div((fix16_mul(mat->arr[0], a.x) + fix16_mul(mat->arr[4], a.y) + fix16_mul(mat->arr[ 8], a.z) + mat->arr[12]), w),
         fix16_div((fix16_mul(mat->arr[1], a.x) + fix16_mul(mat->arr[5], a.y) + fix16_mul(mat->arr[ 9], a.z) + mat->arr[13]), w),
         fix16_div((fix16_mul(mat->arr[2], a.x) + fix16_mul(mat->arr[6], a.y) + fix16_mul(mat->arr[10], a.z) + mat->arr[14]), w)
        );
 }

vec3_t vec3_project_to_ray(vec3_t p, vec3_t r0, vec3_t r1) {
	vec3_t ray = vec3_normalize(vec3_sub(r1, r0));
	fix16_t dp = vec3_dot(vec3_sub(p, r0), ray);
	return vec3_add(r0, vec3_mulf(ray, dp));
}

fix16_t vec3_distance_to_plane(vec3_t p, vec3_t plane_pos, vec3_t plane_normal) {
	fix16_t dot_product = vec3_dot(vec3_sub(plane_pos, p), plane_normal);
	fix16_t norm_dot_product = vec3_dot(vec3_mulf(plane_normal, -1), plane_normal);
	return dot_product / norm_dot_product;
}

vec3_t vec3_reflect(vec3_t incidence, vec3_t normal, fix16_t f) {
	return vec3_add(incidence, vec3_mulf(normal, vec3_dot(normal, vec3_mulf(incidence, -1)) * f));
}

void mat4_set_translation(mat4_t *mat, vec3_t pos) {
	mat->frow[3][0] = pos.x;
	mat->frow[3][1] = pos.y;
	mat->frow[3][2] = pos.z;
}

void mat4_set_yaw_pitch_roll(mat4_t *mat, vec3_t rot) {
	fix16_t sx = sin( rot.x);
	fix16_t sy = sin(-rot.y);
	fix16_t sz = sin(-rot.z);
	fix16_t cx = cos( rot.x);
	fix16_t cy = cos(-rot.y);
	fix16_t cz = cos(-rot.z);

	mat->frow[0][0] = fix16_mul(cy, cz) + fix16_mul(fix16_mul(sx, sy), sz);
	mat->frow[1][0] = fix16_mul(cz, fix16_mul(sx, sy)) - fix16_mul(cy, sz);
	mat->frow[2][0] = fix16_mul(cx, sy);
	mat->frow[0][1] = fix16_mul(cx, sz);
	mat->frow[1][1] = fix16_mul(cx, cz);
	mat->frow[2][1] = -sx;
	mat->frow[0][2] = -fix16_mul(cz, sy) + fix16_mul(cy, fix16_mul(sx, sz));
	mat->frow[1][2] = fix16_mul(cy, fix16_mul(cz, sx)) + fix16_mul(sy, sz);
	mat->frow[2][2] = fix16_mul(cx, cy);
}

void mat4_set_roll_pitch_yaw(mat4_t *mat, vec3_t rot) {
	fix16_t sx = sin( rot.x);
	fix16_t sy = sin(-rot.y);
	fix16_t sz = sin(-rot.z);
	fix16_t cx = cos( rot.x);
	fix16_t cy = cos(-rot.y);
	fix16_t cz = cos(-rot.z);

	mat->frow[0][0] = fix16_mul(cy, cz) - fix16_mul(fix16_mul(sx, sy),sz);
	mat->frow[1][0] = -fix16_mul(cx, sz);
	mat->frow[2][0] = fix16_mul(cz, sy) + fix16_mul(fix16_mul(cy, sx), sz);
	mat->frow[0][1] = fix16_mul(fix16_mul(cz, sx), sy) + fix16_mul(cy, sz);
	mat->frow[1][1] = fix16_mul(cx, cz);
	mat->frow[2][1] = -fix16_mul(fix16_mul(cy, cz), sx) + fix16_mul(sy, sz);
	mat->frow[0][2] = -fix16_mul(cx, sy);
	mat->frow[1][2] = sx;
	mat->frow[2][2] = fix16_mul(cx, cy);
}

void mat4_translate(mat4_t *mat, vec3_t translation) {
	mat->arr[12] = fix16_mul(mat->arr[0], translation.x) + fix16_mul(mat->arr[4], translation.y) + fix16_mul(mat->arr[8], translation.z) + mat->arr[12];
	mat->arr[13] = fix16_mul(mat->arr[1], translation.x) + fix16_mul(mat->arr[5], translation.y) + fix16_mul(mat->arr[9], translation.z) + mat->arr[13];
	mat->arr[14] = fix16_mul(mat->arr[2], translation.x) + fix16_mul(mat->arr[6], translation.y) + fix16_mul(mat->arr[10], translation.z) + mat->arr[14];
	mat->arr[15] = fix16_mul(mat->arr[3], translation.x) + fix16_mul(mat->arr[7], translation.y) + fix16_mul(mat->arr[11], translation.z) + mat->arr[15];
}

void mat4_mul(mat4_t *res, mat4_t *a, mat4_t *b) {
	fix16_mat44_mul(a,b,res);
}
