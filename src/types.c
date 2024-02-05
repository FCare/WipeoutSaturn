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

fix16_t vec3_angle_cos(vec3_t a, vec3_t b) {
	if ((fix16_vec3_length(&a) != FIX16_ZERO) && (fix16_vec3_length(&b) != FIX16_ZERO))
	{
		// printf("[%x,%x,%x]x[%x,%x,%x]\n", a.x, a.y, a.z, b.x, b.y, b.z);
		fix16_vec3_normalize(&a);
		fix16_vec3_normalize(&b);
		fix16_t cosine = vec3_dot(a, b);
		// printf("cosine [%x,%x,%x]x[%x,%x,%x]=>%x\n", a.x, a.y, a.z, b.x, b.y, b.z, cosine);
		return clamp(cosine, -FIX16_ONE, FIX16_ONE);
	}
	return FIX16_ONE;
}

fix16_t vec3_angle(vec3_t a, vec3_t b) {
	return acos(vec3_angle_cos(a, b));
}

// static vec4_t transpose (const fix16_vec4_t *vec) {
// 	static fix16_vec4_t res;
// 	mat44_row_transpose(&vec->comp[0], &res);
// 	return vec4_fix16(res.x, res.y,res.z, res.w);
// }

vec3_t vec3_transform(vec3_t a, mat4_t *mat) {
	vec3_t ret;
	vec4_t ua = (vec4_t){
		.x = a.x,
		.y = a.y,
		.z = a.z,
		.w = FIX16_ONE
	};
	fix16_t w = fix16_vec4_dot(&mat->row[3], &ua);

 	if (w == FIX16_ZERO) {
   	w = FIX16_ONE;
	}

	ua.x = fix16_div(ua.x, w);
	ua.y = fix16_div(ua.y, w);
	ua.z = fix16_div(ua.z, w);
	ua.w = fix16_div(ua.w, w);

	ret.x = fix16_vec4_dot(&mat->row[0], &ua);
	ret.y = fix16_vec4_dot(&mat->row[1], &ua);
	ret.z = fix16_vec4_dot(&mat->row[2], &ua);

	return ret;
 }

vec3_t vec3_rotate(vec3_t a, mat4_t *mat) {
 	vec3_t ret;
 	vec4_t ua = (vec4_t){
 		.x = a.x,
 		.y = a.y,
 		.z = a.z,
 		.w = FIX16_ZERO
 	};

 	ret.x = fix16_vec4_dot(&mat->row[0], &ua);
 	ret.y = fix16_vec4_dot(&mat->row[1], &ua);
 	ret.z = fix16_vec4_dot(&mat->row[2], &ua);

 	return ret;
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
	mat->row[0].w = pos.x;
	mat->row[1].w = pos.y;
	mat->row[2].w = pos.z;
}

void mat4_set_yaw_pitch_roll(mat4_t *mat, vec3_t rot) {
	fix16_t sx = sin( rot.x);
	fix16_t sy = sin(-rot.y);
	fix16_t sz = sin(-rot.z);
	fix16_t cx = cos( rot.x);
	fix16_t cy = cos(-rot.y);
	fix16_t cz = cos(-rot.z);

	mat->row[0].x = fix16_mul(cy, cz) + fix16_mul(fix16_mul(sx, sy), sz);
	mat->row[0].y = fix16_mul(cz, fix16_mul(sx, sy)) - fix16_mul(cy, sz);
	mat->row[0].z = fix16_mul(cx, sy);
	mat->row[1].x = fix16_mul(cx, sz);
	mat->row[1].y = fix16_mul(cx, cz);
	mat->row[1].z = -sx;
	mat->row[2].x = -fix16_mul(cz, sy) + fix16_mul(cy, fix16_mul(sx, sz));
	mat->row[2].y = fix16_mul(cy, fix16_mul(cz, sx)) + fix16_mul(sy, sz);
	mat->row[2].z = fix16_mul(cx, cy);
}

void mat4_set_roll_pitch_yaw(mat4_t *mat, vec3_t rot) {
	fix16_t sx = sin( rot.x);
	fix16_t sy = sin(rot.y);
	fix16_t sz = sin(-rot.z);
	fix16_t cx = cos( rot.x);
	fix16_t cy = cos(rot.y);
	fix16_t cz = cos(-rot.z);

	mat->row[0].x = fix16_mul(cy, cz) - fix16_mul(fix16_mul(sx, sy),sz);
	mat->row[0].y = -fix16_mul(cx, sz);
	mat->row[0].z = fix16_mul(cz, sy) + fix16_mul(fix16_mul(cy, sx), sz);
	mat->row[1].x = fix16_mul(fix16_mul(cz, sx), sy) + fix16_mul(cy, sz);
	mat->row[1].y = fix16_mul(cx, cz);
	mat->row[1].z = -fix16_mul(fix16_mul(cy, cz), sx) + fix16_mul(sy, sz);
	mat->row[2].x = -fix16_mul(cx, sy);
	mat->row[2].y = sx;
	mat->row[2].z = fix16_mul(cx, cy);
}

void mat4_translate(mat4_t *mat, vec3_t translation) {
	mat->row[0].w += translation.x;
	mat->row[1].w += translation.y;
	mat->row[2].w += translation.z;
}

static void mat4_mul(mat4_t *res, mat4_t *A, mat4_t *B) {
	fix16_mat44_mul(A,B,res);
}

void applyTransform(mat4_t *transf, mat4_t *res) {
	mat4_t tmp = *res;
	mat4_mul(res, transf, &tmp);
}

void mat4_rot_inv(mat4_t *res, mat4_t *a) {
	res->arr[0] = a->arr[0];
	res->arr[1] = a->arr[4];
	res->arr[2] = a->arr[8];
	res->arr[3] = -a->arr[3];
	res->arr[4] = a->arr[1];
	res->arr[5] = a->arr[5];
	res->arr[6] = a->arr[9];
	res->arr[7] = -a->arr[7];
	res->arr[8] = a->arr[2];
	res->arr[9] = a->arr[6];
	res->arr[10] = a->arr[10];
	res->arr[11] = -a->arr[11];
	res->arr[12] = -a->arr[12];
	res->arr[13] = -a->arr[13];
	res->arr[14] = -a->arr[14];
	res->arr[15] = a->arr[15];
}

static void print_mat(mat4_t *m __unused) {
  printf("\t[%d %d %d %d]\n\t[%d %d %d %d]\n\t[%d %d %d %d]\n\t[%d %d %d %d]\n",
  (m->arr[0]),
  (m->arr[1]),
  (m->arr[2]),
  (m->arr[3]),
  (m->arr[4]),
  (m->arr[5]),
  (m->arr[6]),
  (m->arr[7]),
  (m->arr[8]),
  (m->arr[9]),
  (m->arr[10]),
  (m->arr[11]),
  (m->arr[12]),
  (m->arr[13]),
  (m->arr[14]),
  (m->arr[15])
  );
}
static void print_vec(vec3_t *v __unused) {
  printf("\t[%d %d %d]\n", v->x, v->y, v->z);
}

void math_test() {
	mat4_t m = mat4_identity();
	printf("Identity =>\n");
	print_mat(&m);
	mat4_set_translation(&m, vec3(1,2,3));
	printf("set_translation =>\n");
	print_mat(&m);
	mat4_translate(&m,vec3(1,2,3));
	printf("translate =>\n");
	print_mat(&m);
	vec3_t test = vec3_transform(vec3(1,0,0), &m);
	printf("transform X =>\n");
	print_vec(&test);
	test = vec3_transform(vec3(0,1,0), &m);
	printf("transform Y =>\n");
	print_vec(&test);
	test = vec3_transform(vec3(0,0,1), &m);
	printf("transform Z =>\n");
	print_vec(&test);
	test = vec3_transform(vec3(1,1,1), &m);
	printf("transform All =>\n");
	print_vec(&test);


	mat4_t yaw = mat4_identity();
	mat4_set_yaw_pitch_roll(&yaw, vec3_fix16(FIX16_ZERO, FIX16_ZERO, PLATFORM_PI));
	printf("Yaw =>\n");
	print_mat(&yaw);
	mat4_t translate = mat4_identity();
	mat4_set_translation(&translate, vec3(1,2,3));
	printf("Translate =>\n");
	print_mat(&translate);
	mat4_t transform;
	mat4_mul(&transform, &translate, &yaw);
	printf("multiplication =>\n");
	print_mat(&transform);

}
