#include <stdio.h>
#include <stdlib.h>

#include "type.h"
#include "object.h"
#include "file.h"
#include "gl.h"
#include "mem.h"

#define SWAP(X) (((X&0xFF)<<8)|(X>>8))
#define SWAP_32(X) (((X&0xFF)<<24)|((X&0xFF00)<<8)|((X&0xFF0000)>>8)|(X>>24))

#define fix16(X) ((uint32_t)(X)<<16)

static texture_t * texture_from_list(texture_list_t *tl, uint16_t index) {
	error_if(index >= tl->len, "Texture %d not in list of len %d", index, tl->len);
	return tl->texture[index];
}

Object *objects_load(char *name, texture_list_t *tl) {
	uint32_t length = 0;
	printf("load: %s\n", name);
	uint8_t *bytes = file_load(name, &length);
	if (!bytes) {
		die("Failed to load file %s\n", name);
	}

	Object *objectList = mem_mark();
	Object *prevObject = NULL;
	uint32_t p = 0;

	while (p < length) {
		Object *object = mem_bump(sizeof(Object));
		if (objectList == NULL) objectList = object;
		if (prevObject) {
			prevObject->next = object;
		}
		prevObject = object;
		for (int i = 0; i < 16; i++) {
			char toto = get_i8(bytes, &p);
			object->name[i] = toto;
		}

		object->vertices_len = get_i16(bytes, &p); p += 2;
		object->vertices = NULL; get_i32(bytes, &p);
		object->normals_len = get_i16(bytes, &p); p += 2;
		object->normals = NULL; get_i32(bytes, &p);
		object->primitives_len = get_i16(bytes, &p); p += 2;
		object->primitives = NULL; get_i32(bytes, &p);
		get_i32(bytes, &p);
		get_i32(bytes, &p);
		get_i32(bytes, &p); // Skeleton ref
		object->extent = get_i32(bytes, &p);
		object->flags = get_i16(bytes, &p); p += 2;
		object->next = NULL; get_i32(bytes, &p);

		p += 3 * 3 * 2; // relative rot matrix
		p += 2; // padding

		object->origin.x = get_i32(bytes, &p);
		object->origin.y = get_i32(bytes, &p);
		object->origin.z = get_i32(bytes, &p);

		p += 3 * 3 * 2; // absolute rot matrix
		p += 2; // padding
		p += 3 * 4; // absolute translation matrix
		p += 2; // skeleton update flag
		p += 2; // padding
		p += 4; // skeleton super
		p += 4; // skeleton sub
		p += 4; // skeleton next

		object->radius = 0;
		object->vertices = mem_bump(object->vertices_len * sizeof(vec3_t));
		for (int i = 0; i < object->vertices_len; i++) {
			int val = get_i16(bytes, &p);
			object->vertices[i].x = val;
			val = get_i16(bytes, &p);
			object->vertices[i].y = val;
			val = get_i16(bytes, &p);
			object->vertices[i].z = val;
			p += 2; // padding

			if (abs(object->vertices[i].x) > object->radius) {
				object->radius = abs(object->vertices[i].x);
			}
			if (abs(object->vertices[i].y) > object->radius) {
				object->radius = abs(object->vertices[i].y);
			}
			if (abs(object->vertices[i].z) > object->radius) {
				object->radius = abs(object->vertices[i].z);
			}
		}

		object->normals = mem_bump(object->normals_len * sizeof(vec3_t));
		for (int i = 0; i < object->normals_len; i++) {
			object->normals[i].x = get_i16(bytes, &p);
			object->normals[i].y = get_i16(bytes, &p);
			object->normals[i].z = get_i16(bytes, &p);
			p += 2; // padding
		}
		object->primitives = mem_mark();
		for (int i = 0; i < object->primitives_len; i++) {
			Prm prm;
			int16_t prm_type = get_i16(bytes, &p);
			int16_t prm_flag = get_i16(bytes, &p);

			switch (prm_type) {
			case PRM_TYPE_F3:
				prm.ptr = mem_bump(sizeof(F3));
				prm.f3->coords[0] = get_i16(bytes, &p);
				prm.f3->coords[1] = get_i16(bytes, &p);
				prm.f3->coords[2] = get_i16(bytes, &p);
				prm.f3->pad1 = get_i16(bytes, &p);
				prm.f3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_F4:
				prm.ptr = mem_bump(sizeof(F4));
				prm.f4->coords[0] = get_i16(bytes, &p);
				prm.f4->coords[1] = get_i16(bytes, &p);
				prm.f4->coords[2] = get_i16(bytes, &p);
				prm.f4->coords[3] = get_i16(bytes, &p);
				prm.f4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_FT3:
				prm.ptr = mem_bump(sizeof(FT3));
				prm.ft3->coords[0] = get_i16(bytes, &p);
				prm.ft3->coords[1] = get_i16(bytes, &p);
				prm.ft3->coords[2] = get_i16(bytes, &p);

				prm.ft3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.ft3->cba = get_i16(bytes, &p);
				prm.ft3->tsb = get_i16(bytes, &p);
				prm.ft3->u0 = get_i8(bytes, &p);
				prm.ft3->v0 = get_i8(bytes, &p);
				prm.ft3->u1 = get_i8(bytes, &p);
				prm.ft3->v1 = get_i8(bytes, &p);
				prm.ft3->u2 = get_i8(bytes, &p);
				prm.ft3->v2 = get_i8(bytes, &p);

				prm.ft3->pad1 = get_i16(bytes, &p);
				prm.ft3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_FT4:
				prm.ptr = mem_bump(sizeof(FT4));
				prm.ft4->coords[0] = get_i16(bytes, &p);
				prm.ft4->coords[1] = get_i16(bytes, &p);
				prm.ft4->coords[2] = get_i16(bytes, &p);
				prm.ft4->coords[3] = get_i16(bytes, &p);

				prm.ft4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.ft4->cba = get_i16(bytes, &p);
				prm.ft4->tsb = get_i16(bytes, &p);
				prm.ft4->u0 = get_i8(bytes, &p);
				prm.ft4->v0 = get_i8(bytes, &p);
				prm.ft4->u1 = get_i8(bytes, &p);
				prm.ft4->v1 = get_i8(bytes, &p);
				prm.ft4->u2 = get_i8(bytes, &p);
				prm.ft4->v2 = get_i8(bytes, &p);
				prm.ft4->u3 = get_i8(bytes, &p);
				prm.ft4->v3 = get_i8(bytes, &p);
				prm.ft4->pad1 = get_i16(bytes, &p);
				prm.ft4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_G3:
				prm.ptr = mem_bump(sizeof(G3));
				prm.g3->coords[0] = get_i16(bytes, &p);
				prm.g3->coords[1] = get_i16(bytes, &p);
				prm.g3->coords[2] = get_i16(bytes, &p);
				prm.g3->pad1 = get_i16(bytes, &p);
				prm.g3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.g3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.g3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_G4:
				prm.ptr = mem_bump(sizeof(G4));
				prm.g4->coords[0] = get_i16(bytes, &p);
				prm.g4->coords[1] = get_i16(bytes, &p);
				prm.g4->coords[2] = get_i16(bytes, &p);
				prm.g4->coords[3] = get_i16(bytes, &p);
				prm.g4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.g4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_GT3:
				prm.ptr = mem_bump(sizeof(GT3));
				prm.gt3->coords[0] = get_i16(bytes, &p);
				prm.gt3->coords[1] = get_i16(bytes, &p);
				prm.gt3->coords[2] = get_i16(bytes, &p);

				prm.gt3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.gt3->cba = get_i16(bytes, &p);
				prm.gt3->tsb = get_i16(bytes, &p);
				prm.gt3->u0 = get_i8(bytes, &p);
				prm.gt3->v0 = get_i8(bytes, &p);
				prm.gt3->u1 = get_i8(bytes, &p);
				prm.gt3->v1 = get_i8(bytes, &p);
				prm.gt3->u2 = get_i8(bytes, &p);
				prm.gt3->v2 = get_i8(bytes, &p);
				prm.gt3->pad1 = get_i16(bytes, &p);
				prm.gt3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_GT4:
				prm.ptr = mem_bump(sizeof(GT4));
				prm.gt4->coords[0] = get_i16(bytes, &p);
				prm.gt4->coords[1] = get_i16(bytes, &p);
				prm.gt4->coords[2] = get_i16(bytes, &p);
				prm.gt4->coords[3] = get_i16(bytes, &p);

				prm.gt4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.gt4->cba = get_i16(bytes, &p);
				prm.gt4->tsb = get_i16(bytes, &p);
				prm.gt4->u0 = get_i8(bytes, &p);
				prm.gt4->v0 = get_i8(bytes, &p);
				prm.gt4->u1 = get_i8(bytes, &p);
				prm.gt4->v1 = get_i8(bytes, &p);
				prm.gt4->u2 = get_i8(bytes, &p);
				prm.gt4->v2 = get_i8(bytes, &p);
				prm.gt4->u3 = get_i8(bytes, &p);
				prm.gt4->v3 = get_i8(bytes, &p);
				prm.gt4->pad1 = get_i16(bytes, &p);
				prm.gt4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.gt4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;


			case PRM_TYPE_LSF3:
				prm.ptr = mem_bump(sizeof(LSF3));
				prm.lsf3->coords[0] = get_i16(bytes, &p);
				prm.lsf3->coords[1] = get_i16(bytes, &p);
				prm.lsf3->coords[2] = get_i16(bytes, &p);
				prm.lsf3->normal = get_i16(bytes, &p);
				prm.lsf3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSF4:
				prm.ptr = mem_bump(sizeof(LSF4));
				prm.lsf4->coords[0] = get_i16(bytes, &p);
				prm.lsf4->coords[1] = get_i16(bytes, &p);
				prm.lsf4->coords[2] = get_i16(bytes, &p);
				prm.lsf4->coords[3] = get_i16(bytes, &p);
				prm.lsf4->normal = get_i16(bytes, &p);
				prm.lsf4->pad1 = get_i16(bytes, &p);
				prm.lsf4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSFT3:
				prm.ptr = mem_bump(sizeof(LSFT3));
				prm.lsft3->coords[0] = get_i16(bytes, &p);
				prm.lsft3->coords[1] = get_i16(bytes, &p);
				prm.lsft3->coords[2] = get_i16(bytes, &p);
				prm.lsft3->normal = get_i16(bytes, &p);

				prm.lsft3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsft3->cba = get_i16(bytes, &p);
				prm.lsft3->tsb = get_i16(bytes, &p);
				prm.lsft3->u0 = get_i8(bytes, &p);
				prm.lsft3->v0 = get_i8(bytes, &p);
				prm.lsft3->u1 = get_i8(bytes, &p);
				prm.lsft3->v1 = get_i8(bytes, &p);
				prm.lsft3->u2 = get_i8(bytes, &p);
				prm.lsft3->v2 = get_i8(bytes, &p);
				prm.lsft3->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSFT4:
				prm.ptr = mem_bump(sizeof(LSFT4));
				prm.lsft4->coords[0] = get_i16(bytes, &p);
				prm.lsft4->coords[1] = get_i16(bytes, &p);
				prm.lsft4->coords[2] = get_i16(bytes, &p);
				prm.lsft4->coords[3] = get_i16(bytes, &p);
				prm.lsft4->normal = get_i16(bytes, &p);

				prm.lsft4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsft4->cba = get_i16(bytes, &p);
				prm.lsft4->tsb = get_i16(bytes, &p);
				prm.lsft4->u0 = get_i8(bytes, &p);
				prm.lsft4->v0 = get_i8(bytes, &p);
				prm.lsft4->u1 = get_i8(bytes, &p);
				prm.lsft4->v1 = get_i8(bytes, &p);
				prm.lsft4->u2 = get_i8(bytes, &p);
				prm.lsft4->v2 = get_i8(bytes, &p);
				prm.lsft4->u3 = get_i8(bytes, &p);
				prm.lsft4->v3 = get_i8(bytes, &p);
				prm.lsft4->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSG3:
				prm.ptr = mem_bump(sizeof(LSG3));
				prm.lsg3->coords[0] = get_i16(bytes, &p);
				prm.lsg3->coords[1] = get_i16(bytes, &p);
				prm.lsg3->coords[2] = get_i16(bytes, &p);
				prm.lsg3->normals[0] = get_i16(bytes, &p);
				prm.lsg3->normals[1] = get_i16(bytes, &p);
				prm.lsg3->normals[2] = get_i16(bytes, &p);
				prm.lsg3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSG4:
				prm.ptr = mem_bump(sizeof(LSG4));
				prm.lsg4->coords[0] = get_i16(bytes, &p);
				prm.lsg4->coords[1] = get_i16(bytes, &p);
				prm.lsg4->coords[2] = get_i16(bytes, &p);
				prm.lsg4->coords[3] = get_i16(bytes, &p);
				prm.lsg4->normals[0] = get_i16(bytes, &p);
				prm.lsg4->normals[1] = get_i16(bytes, &p);
				prm.lsg4->normals[2] = get_i16(bytes, &p);
				prm.lsg4->normals[3] = get_i16(bytes, &p);
				prm.lsg4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsg4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSGT3:
				prm.ptr = mem_bump(sizeof(LSGT3));
				prm.lsgt3->coords[0] = get_i16(bytes, &p);
				prm.lsgt3->coords[1] = get_i16(bytes, &p);
				prm.lsgt3->coords[2] = get_i16(bytes, &p);
				prm.lsgt3->normals[0] = get_i16(bytes, &p);
				prm.lsgt3->normals[1] = get_i16(bytes, &p);
				prm.lsgt3->normals[2] = get_i16(bytes, &p);

				prm.lsgt3->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsgt3->cba = get_i16(bytes, &p);
				prm.lsgt3->tsb = get_i16(bytes, &p);
				prm.lsgt3->u0 = get_i8(bytes, &p);
				prm.lsgt3->v0 = get_i8(bytes, &p);
				prm.lsgt3->u1 = get_i8(bytes, &p);
				prm.lsgt3->v1 = get_i8(bytes, &p);
				prm.lsgt3->u2 = get_i8(bytes, &p);
				prm.lsgt3->v2 = get_i8(bytes, &p);
				prm.lsgt3->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt3->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt3->color[2] = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_LSGT4:
				prm.ptr = mem_bump(sizeof(LSGT4));
				prm.lsgt4->coords[0] = get_i16(bytes, &p);
				prm.lsgt4->coords[1] = get_i16(bytes, &p);
				prm.lsgt4->coords[2] = get_i16(bytes, &p);
				prm.lsgt4->coords[3] = get_i16(bytes, &p);
				prm.lsgt4->normals[0] = get_i16(bytes, &p);
				prm.lsgt4->normals[1] = get_i16(bytes, &p);
				prm.lsgt4->normals[2] = get_i16(bytes, &p);
				prm.lsgt4->normals[3] = get_i16(bytes, &p);

				prm.lsgt4->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.lsgt4->cba = get_i16(bytes, &p);
				prm.lsgt4->tsb = get_i16(bytes, &p);
				prm.lsgt4->u0 = get_i8(bytes, &p);
				prm.lsgt4->v0 = get_i8(bytes, &p);
				prm.lsgt4->u1 = get_i8(bytes, &p);
				prm.lsgt4->v1 = get_i8(bytes, &p);
				prm.lsgt4->u2 = get_i8(bytes, &p);
				prm.lsgt4->v2 = get_i8(bytes, &p);
				prm.lsgt4->pad1 = get_i16(bytes, &p);
				prm.lsgt4->color[0] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[1] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[2] = rgba_from_u32(get_u32(bytes, &p));
				prm.lsgt4->color[3] = rgba_from_u32(get_u32(bytes, &p));
				break;


			case PRM_TYPE_TSPR:
			case PRM_TYPE_BSPR:
				prm.ptr = mem_bump(sizeof(SPR));
				prm.spr->coord = get_i16(bytes, &p);
				prm.spr->width = get_i16(bytes, &p);
				prm.spr->height = get_i16(bytes, &p);
				prm.spr->texture = texture_from_list(tl, get_i16(bytes, &p));
				prm.spr->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_SPLINE:
				prm.ptr = mem_bump(sizeof(Spline));
				prm.spline->control1.x = get_i32(bytes, &p);
				prm.spline->control1.y = get_i32(bytes, &p);
				prm.spline->control1.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->position.x = get_i32(bytes, &p);
				prm.spline->position.y = get_i32(bytes, &p);
				prm.spline->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->control2.x = get_i32(bytes, &p);
				prm.spline->control2.y = get_i32(bytes, &p);
				prm.spline->control2.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spline->color = rgba_from_u32(get_u32(bytes, &p));
				break;

			case PRM_TYPE_POINT_LIGHT:
				prm.ptr = mem_bump(sizeof(PointLight));
				prm.pointLight->position.x = get_i32(bytes, &p);
				prm.pointLight->position.y = get_i32(bytes, &p);
				prm.pointLight->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.pointLight->color = rgba_from_u32(get_u32(bytes, &p));
				prm.pointLight->startFalloff = get_i16(bytes, &p);
				prm.pointLight->endFalloff = get_i16(bytes, &p);
				break;

			case PRM_TYPE_SPOT_LIGHT:
				prm.ptr = mem_bump(sizeof(SpotLight));
				prm.spotLight->position.x = get_i32(bytes, &p);
				prm.spotLight->position.y = get_i32(bytes, &p);
				prm.spotLight->position.z = get_i32(bytes, &p);
				p += 4; // padding
				prm.spotLight->direction.x = get_i16(bytes, &p);
				prm.spotLight->direction.y = get_i16(bytes, &p);
				prm.spotLight->direction.z = get_i16(bytes, &p);
				p += 2; // padding
				prm.spotLight->color = rgba_from_u32(get_u32(bytes, &p));
				prm.spotLight->startFalloff = get_i16(bytes, &p);
				prm.spotLight->endFalloff = get_i16(bytes, &p);
				prm.spotLight->coneAngle = get_i16(bytes, &p);
				prm.spotLight->spreadAngle = get_i16(bytes, &p);
				break;

			case PRM_TYPE_INFINITE_LIGHT:
				prm.ptr = mem_bump(sizeof(InfiniteLight));
				prm.infiniteLight->direction.x = get_i16(bytes, &p);
				prm.infiniteLight->direction.y = get_i16(bytes, &p);
				prm.infiniteLight->direction.z = get_i16(bytes, &p);
				p += 2; // padding
				prm.infiniteLight->color = rgba_from_u32(get_u32(bytes, &p));
				break;


			default:
				die("bad primitive type %x \n", prm_type);
			} // switch
			if (object->primitives == NULL) object->primitives = (Primitive *)prm.ptr;
			prm.f3->type = prm_type;
			prm.f3->flag = prm_flag;
		} // each prim
	} // each object

	free(bytes);
	return objectList;
}

void write_16(uint16_t val, FILE *f) {
	uint16_t tmp = SWAP(val);
	// int pos = ftell(f);
	// printf("a2 pos %x=>%x\n", pos, (pos+0x1)&~0x1);
	// fseek(f, (pos+0x1)&~0x1, SEEK_SET); //Align on 16 bits
	fwrite(&tmp, 2, sizeof(uint8_t), f);
}

void write_32(uint32_t val, FILE *f) {
	uint32_t tmp = SWAP_32(val);
	// int pos = ftell(f);
	// printf("a4 pos %x=>%x\n", pos, (pos+0x3)&~0x3);
	// fseek(f, (pos+0x3)&~0x3, SEEK_SET); //Align on 32 bits
	fwrite(&tmp, 4, sizeof(uint8_t), f);
}

void write_fix(uint16_t val, FILE *f) {
	uint32_t tmp = SWAP_32(fix16(val));
	// int pos = ftell(f);
	// printf("a4 pos %x=>%x\n", pos, (pos+0x3)&~0x3);
	// fseek(f, (pos+0x3)&~0x3, SEEK_SET); //Align on 32 bits
	fwrite(&tmp, 4, sizeof(uint8_t), f);
}

void pad(FILE *f) {
	uint16_t z = 0;
	// fwrite(<&z, 2, sizeof(uint8_t), f);
}

uint16_t getNbCharacters(Object *obj) {
	uint16_t nb_texture = 0;
	Prm poly = {.primitive = obj->primitives};
	for (int i = 0; i < obj->primitives_len; i++) {
		switch (poly.primitive->type) {
			case PRM_TYPE_F3:
				poly.f3 += 1;
			break;
			case PRM_TYPE_F4:
				poly.f4 += 1;
			break;
			case PRM_TYPE_FT3:
				nb_texture++;
				poly.ft3 += 1;
			break;
			case PRM_TYPE_FT4:
				nb_texture++;
				poly.ft4 += 1;
			break;
			case PRM_TYPE_G3:
				poly.g3 += 1;
			break;
			case PRM_TYPE_G4:
				poly.g4 += 1;
			break;
			case PRM_TYPE_GT3:
				nb_texture++;
				poly.gt3 += 1;
			break;
			case PRM_TYPE_GT4:
				nb_texture++;
				poly.gt4 += 1;
			break;
			case PRM_TYPE_LSF3:
				poly.lsf3 += 1;
			break;
			case PRM_TYPE_LSF4:
				poly.lsf4 += 1;
			break;
			case PRM_TYPE_LSFT3:
				nb_texture++;
				poly.lsft3 += 1;
			break;
			case PRM_TYPE_LSFT4:
				nb_texture++;
				poly.lsft4 += 1;
			break;
			case PRM_TYPE_LSG3:
				poly.lsg3 += 1;
			break;
			case PRM_TYPE_LSG4:
				poly.lsg4 += 1;
			break;
			case PRM_TYPE_LSGT3:
				nb_texture++;
				poly.lsgt3 += 1;
			break;
			case PRM_TYPE_LSGT4:
				nb_texture++;
				poly.lsgt4 += 1;
			break;
			case PRM_TYPE_TSPR:
			case PRM_TYPE_BSPR:
				nb_texture++;
				poly.spr += 1;
			break;
			case PRM_TYPE_SPLINE:
				poly.spline += 1;
			break;
			case PRM_TYPE_POINT_LIGHT:
				poly.pointLight += 1;
			break;
			case PRM_TYPE_SPOT_LIGHT:
				poly.spotLight += 1;
			break;
			case PRM_TYPE_INFINITE_LIGHT:
				poly.infiniteLight += 1;
			break;
			default:
				die("bad primitive type\n");
		}
	}
	return nb_texture;
}

void objects_save(const char *objectPath, const char *texturePath, Object** model, int nb_objects, texture_list_t *textures)
{
	FILE *fobj = fopen(objectPath, "wb+");
	FILE *ftex = fopen(texturePath, "wb+");
	int nb_texture = 0;
	write_16((uint16_t)nb_objects, fobj);
	printf("Output Nb Objects = %d\n", nb_objects);
	for (int n=0; n<nb_objects; n++) {
		uint16_t tmp;
		uint32_t tmp32;
		Object * obj = model[n];
		fwrite(obj->name, 16, sizeof(char), fobj);
		write_16((uint16_t)obj->vertices_len, fobj);
		write_16((uint16_t)obj->normals_len, fobj);
		write_16((uint16_t)obj->primitives_len, fobj);
		write_16((uint16_t)obj->flags, fobj);
		write_fix((uint16_t)obj->origin.x, fobj);
		write_fix((uint16_t)obj->origin.y, fobj);
		write_fix((uint16_t)obj->origin.z, fobj);

		Prm poly = {.primitive = obj->primitives};
		printf("Output primitives %d @0x%x\n",obj->primitives_len, ftell(fobj));
		for (int i = 0; i < obj->primitives_len; i++) {
			write_16((uint16_t)poly.primitive->type, fobj);
			switch (poly.primitive->type) {
				case PRM_TYPE_F3:
					write_16((uint16_t)poly.f3->flag, fobj);
					write_16((uint16_t)poly.f3->coords[0], fobj);
					write_16((uint16_t)poly.f3->coords[1], fobj);
					write_16((uint16_t)poly.f3->coords[2], fobj);
					write_16(convert_to_rgb(poly.f3->color), fobj);
					pad(fobj);
					poly.f3 += 1;
				break;
				case PRM_TYPE_F4:
					write_16((uint16_t)poly.f4->flag, fobj);
					write_16((uint16_t)poly.f4->coords[0], fobj);
					write_16((uint16_t)poly.f4->coords[1], fobj);
					write_16((uint16_t)poly.f4->coords[2], fobj);
					write_16((uint16_t)poly.f4->coords[3], fobj);
					write_16(convert_to_rgb(poly.f4->color), fobj);
					poly.f4 += 1;
				break;
				case PRM_TYPE_FT3:
					write_16((uint16_t)poly.ft3->flag, fobj);
					write_16((uint16_t)poly.ft3->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.ft3->conv->palette_id, fobj);
					write_16((uint16_t)poly.ft3->coords[0], fobj);
					write_16((uint16_t)poly.ft3->coords[1], fobj);
					write_16((uint16_t)poly.ft3->coords[2], fobj);
					write_16(convert_to_rgb(poly.ft3->color), fobj);
					poly.ft3 += 1;
				break;
				case PRM_TYPE_FT4:
					printf("FT4 flasg is 0x%x\n",poly.ft4->flag);
					write_16((uint16_t)poly.ft4->flag, fobj);
					write_16((uint16_t)poly.ft4->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.ft4->conv->palette_id, fobj);
					write_16((uint16_t)poly.ft4->coords[0], fobj);
					write_16((uint16_t)poly.ft4->coords[1], fobj);
					write_16((uint16_t)poly.ft4->coords[2], fobj);
					write_16((uint16_t)poly.ft4->coords[3], fobj);
					write_16((uint16_t)convert_to_rgb(poly.ft4->color), fobj);
					pad(fobj);
					poly.ft4 += 1;
				break;
				case PRM_TYPE_G3:
					write_16((uint16_t)poly.g3->flag, fobj);
					write_16((uint16_t)poly.g3->coords[0], fobj);
					write_16((uint16_t)poly.g3->coords[1], fobj);
					write_16((uint16_t)poly.g3->coords[2], fobj);
					write_16((uint16_t)convert_to_rgb(poly.g3->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.g3->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.g3->color[2]), fobj);
					pad(fobj);
					poly.g3 += 1;
				break;
				case PRM_TYPE_G4:
					write_16((uint16_t)poly.g4->flag, fobj);
					write_16((uint16_t)poly.g4->coords[0], fobj);
					write_16((uint16_t)poly.g4->coords[1], fobj);
					write_16((uint16_t)poly.g4->coords[2], fobj);
					write_16((uint16_t)poly.g4->coords[3], fobj);
					write_16((uint16_t)convert_to_rgb(poly.g4->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.g4->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.g4->color[2]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.g4->color[3]), fobj);
					pad(fobj);
					poly.g4 += 1;
				break;
				case PRM_TYPE_GT3:
					write_16((uint16_t)poly.gt3->flag, fobj);
					write_16((uint16_t)poly.gt3->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.gt3->conv->palette_id, fobj);
					write_16((uint16_t)poly.gt3->coords[0], fobj);
					write_16((uint16_t)poly.gt3->coords[1], fobj);
					write_16((uint16_t)poly.gt3->coords[2], fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt3->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt3->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt3->color[2]), fobj);
					poly.gt3 += 1;
				break;
				case PRM_TYPE_GT4:
					write_16((uint16_t)poly.gt4->flag, fobj);
					write_16((uint16_t)poly.gt4->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.gt4->conv->palette_id, fobj);
					write_16((uint16_t)poly.gt4->coords[0], fobj);
					write_16((uint16_t)poly.gt4->coords[1], fobj);
					write_16((uint16_t)poly.gt4->coords[2], fobj);
					write_16((uint16_t)poly.gt4->coords[3], fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt4->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt4->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt4->color[2]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.gt4->color[3]), fobj);
					poly.gt4 += 1;
				break;
				case PRM_TYPE_LSF3:
					write_16((uint16_t)poly.lsf3->flag, fobj);
					write_16((uint16_t)poly.lsf3->coords[0], fobj);
					write_16((uint16_t)poly.lsf3->coords[1], fobj);
					write_16((uint16_t)poly.lsf3->coords[2], fobj);
					write_fix((uint16_t)poly.lsf3->normal, fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsf3->color), fobj);
					pad(fobj);
					poly.lsf3 += 1;
				break;
				case PRM_TYPE_LSF4:
					write_16((uint16_t)poly.lsf4->flag, fobj);
					write_16((uint16_t)poly.lsf4->coords[0], fobj);
					write_16((uint16_t)poly.lsf4->coords[1], fobj);
					write_16((uint16_t)poly.lsf4->coords[2], fobj);
					write_16((uint16_t)poly.lsf4->coords[3], fobj);
					write_fix((uint16_t)poly.lsf4->normal, fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsf4->color), fobj);
					poly.lsf4 += 1;
				break;
				case PRM_TYPE_LSFT3:
					write_16((uint16_t)poly.lsft3->flag, fobj);
					write_16((uint16_t)poly.lsft3->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsft3->conv->palette_id, fobj);
					write_16((uint16_t)poly.lsft3->coords[0], fobj);
					write_16((uint16_t)poly.lsft3->coords[1], fobj);
					write_16((uint16_t)poly.lsft3->coords[2], fobj);
					write_fix((uint16_t)poly.lsft3->normal, fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsft3->color), fobj);
					poly.lsft3 += 1;
				break;
				case PRM_TYPE_LSFT4:
					write_16((uint16_t)poly.lsft4->flag, fobj);
					write_16((uint16_t)poly.lsft4->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsft4->conv->palette_id, fobj);
					write_16((uint16_t)poly.lsft4->coords[0], fobj);
					write_16((uint16_t)poly.lsft4->coords[1], fobj);
					write_16((uint16_t)poly.lsft4->coords[2], fobj);
					write_16((uint16_t)poly.lsft4->coords[3], fobj);
					write_fix((uint16_t)poly.lsft4->normal, fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsft4->color), fobj);
					pad(fobj);
					poly.lsft4 += 1;
				break;
				case PRM_TYPE_LSG3:
					write_16((uint16_t)poly.lsg3->flag, fobj);
					write_16((uint16_t)poly.lsg3->coords[0], fobj);
					write_16((uint16_t)poly.lsg3->coords[1], fobj);
					write_16((uint16_t)poly.lsg3->coords[2], fobj);
					write_fix((uint16_t)poly.lsg3->normals[0], fobj);
					write_fix((uint16_t)poly.lsg3->normals[1], fobj);
					write_fix((uint16_t)poly.lsg3->normals[2], fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg3->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg3->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg3->color[2]), fobj);
					pad(fobj);
					poly.lsg3 += 1;
				break;
				case PRM_TYPE_LSG4:
					write_16((uint16_t)poly.lsg4->flag, fobj);
					write_16((uint16_t)poly.lsg4->coords[0], fobj);
					write_16((uint16_t)poly.lsg4->coords[1], fobj);
					write_16((uint16_t)poly.lsg4->coords[2], fobj);
					write_16((uint16_t)poly.lsg4->coords[3], fobj);
					write_fix((uint16_t)poly.lsg4->normals[0], fobj);
					write_fix((uint16_t)poly.lsg4->normals[1], fobj);
					write_fix((uint16_t)poly.lsg4->normals[2], fobj);
					write_fix((uint16_t)poly.lsg4->normals[3], fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg4->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg4->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg4->color[2]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsg4->color[3]), fobj);
					pad(fobj);
					poly.lsg4 += 1;
				break;
				case PRM_TYPE_LSGT3:
					write_16((uint16_t)poly.lsgt3->flag, fobj);
					write_16((uint16_t)poly.lsgt3->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsgt3->conv->palette_id, fobj);
					write_16((uint16_t)poly.lsgt3->coords[0], fobj);
					write_16((uint16_t)poly.lsgt3->coords[1], fobj);
					write_16((uint16_t)poly.lsgt3->coords[2], fobj);
					write_fix((uint16_t)poly.lsgt3->normals[0], fobj);
					write_fix((uint16_t)poly.lsgt3->normals[1], fobj);
					write_fix((uint16_t)poly.lsgt3->normals[2], fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt3->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt3->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt3->color[2]), fobj);
					poly.lsgt3 += 1;
				break;
				case PRM_TYPE_LSGT4:
					write_16((uint16_t)poly.lsgt4->flag, fobj);
					write_16((uint16_t)poly.lsgt4->coords[0], fobj);
					write_16((uint16_t)poly.lsgt4->coords[1], fobj);
					write_16((uint16_t)poly.lsgt4->coords[2], fobj);
					write_16((uint16_t)poly.lsgt4->coords[3], fobj);
					write_fix((uint16_t)poly.lsgt4->normals[0], fobj);
					write_fix((uint16_t)poly.lsgt4->normals[1], fobj);
					write_fix((uint16_t)poly.lsgt4->normals[2], fobj);
					write_fix((uint16_t)poly.lsgt4->normals[3], fobj);
					write_16((uint16_t)poly.lsgt4->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsgt4->conv->palette_id, fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt4->color[0]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt4->color[1]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt4->color[2]), fobj);
					write_16((uint16_t)convert_to_rgb(poly.lsgt4->color[3]), fobj);
					poly.lsgt4 += 1;
				break;
				case PRM_TYPE_TSPR:
				case PRM_TYPE_BSPR:
					write_16((uint16_t)poly.spr->flag, fobj);
					write_16((uint16_t)poly.spr->conv->id, fobj);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.spr->conv->palette_id, fobj);
					write_16((uint16_t)poly.spr->coord, fobj);
					write_16((uint16_t)poly.spr->width, fobj);
					write_16((uint16_t)poly.spr->height, fobj);
					write_16((uint16_t)convert_to_rgb(poly.spr->color), fobj);
					poly.spr += 1;
				break;
				case PRM_TYPE_SPLINE:
					write_16((uint16_t)poly.spline->flag, fobj);
					write_fix((uint32_t)poly.spline->control1.x, fobj);
					write_fix((uint32_t)poly.spline->control1.y, fobj);
					write_fix((uint32_t)poly.spline->control1.z, fobj);
					write_fix((uint32_t)poly.spline->position.x, fobj);
					write_fix((uint32_t)poly.spline->position.y, fobj);
					write_fix((uint32_t)poly.spline->position.z, fobj);
					write_fix((uint32_t)poly.spline->control2.x, fobj);
					write_fix((uint32_t)poly.spline->control2.y, fobj);
					write_fix((uint32_t)poly.spline->control2.z, fobj);
					write_16((uint16_t)convert_to_rgb(poly.spline->color), fobj);
					poly.spline += 1;
				break;
				case PRM_TYPE_POINT_LIGHT:
					write_16((uint16_t)poly.pointLight->flag, fobj);
					write_fix((uint32_t)poly.pointLight->position.x, fobj);
					write_fix((uint32_t)poly.pointLight->position.y, fobj);
					write_fix((uint32_t)poly.pointLight->position.z, fobj);
					write_16((uint16_t)convert_to_rgb(poly.pointLight->color), fobj);
					write_16((uint16_t)poly.pointLight->startFalloff, fobj);
					write_16((uint16_t)poly.pointLight->endFalloff, fobj);
					poly.pointLight += 1;
				break;
				case PRM_TYPE_SPOT_LIGHT:
					write_16((uint16_t)poly.spotLight->flag, fobj);
					write_fix((uint32_t)poly.spotLight->position.x, fobj);
					write_fix((uint32_t)poly.spotLight->position.y, fobj);
					write_fix((uint32_t)poly.spotLight->position.z, fobj);
					write_fix((uint16_t)poly.spotLight->direction.x, fobj);
					write_fix((uint16_t)poly.spotLight->direction.y, fobj);
					write_fix((uint16_t)poly.spotLight->direction.z, fobj);
					write_16((uint16_t)convert_to_rgb(poly.spotLight->color), fobj);
					write_16((uint16_t)poly.spotLight->startFalloff, fobj);
					write_16((uint16_t)poly.spotLight->endFalloff, fobj);
					write_16((uint16_t)poly.spotLight->coneAngle, fobj);
					write_16((uint16_t)poly.spotLight->spreadAngle, fobj);
					poly.spotLight += 1;
				break;
				case PRM_TYPE_INFINITE_LIGHT:
				write_16((uint16_t)poly.infiniteLight->flag, fobj);
					write_fix((uint16_t)poly.infiniteLight->direction.x, fobj);
					write_fix((uint16_t)poly.infiniteLight->direction.y, fobj);
					write_fix((uint16_t)poly.infiniteLight->direction.z, fobj);
					write_16((uint16_t)convert_to_rgb(poly.infiniteLight->color), fobj);
					poly.infiniteLight += 1;
				break;
				default:
					die("bad primitive type\n");
			}
		}
		for (int i=0; i<obj->vertices_len; i++) {
			write_fix((uint16_t)obj->vertices[i].x, fobj);
			write_fix((uint16_t)obj->vertices[i].y, fobj);
			write_fix((uint16_t)obj->vertices[i].z, fobj);
		}
		for (int i=0; i<obj->normals_len; i++) {
			write_fix((uint16_t)obj->normals[i].x, fobj);
			write_fix((uint16_t)obj->normals[i].y, fobj);
			write_fix((uint16_t)obj->normals[i].z, fobj);
		}
	}

	write_16((uint16_t)textures->len, ftex);
	printf("Output palette nb %d\n",textures->len);
	for (int i = 0; i < textures->len; i++) {
		printf("Palette[%d] @0x%x\n", i, ftell(ftex));
		texture_t *tex = textures->texture[i];
		write_16((uint16_t)tex->format, ftex);
		write_16((uint16_t)1, ftex);
		switch(tex->format) {
			case COLOR_BANK_16_COL:
			case LOOKUP_TABLE_16_COL:
				write_16((uint16_t)16, ftex);
				for (int j=0; j<16; j++) write_16((uint16_t)tex->palette.pixels[j], ftex);
				break;
			case COLOR_BANK_64_COL:
				write_16((uint16_t)64, ftex);
				printf("64 should never happen\n");
				exit(-1);
				for (int j=0; j<64; j++) write_16((uint16_t)tex->palette.pixels[j], ftex);
				break;
			case COLOR_BANK_128_COL:
			printf("128 should never happen\n");
			exit(-1);
			write_16((uint16_t)128, ftex);
				for (int j=0; j<128; j++) write_16((uint16_t)tex->palette.pixels[j], ftex);
				break;
			case COLOR_BANK_256_COL:
			printf("256 should never happen\n");
			exit(-1);
			write_16((uint16_t)256, ftex);
				for (int j=0; j<256; j++) write_16((uint16_t)tex->palette.pixels[j], ftex);
				break;
			default:
				break;
		}
	}
	printf("Object %d is at 0x%x\n", nb_objects, ftell(ftex));
	write_16((uint16_t)nb_objects, ftex);
	for (int n=0; n<nb_objects; n++) {
		uint16_t tmp;
		uint32_t tmp32;
		Object * obj = model[n];
		int nb_texture = getNbCharacters(obj);
		write_16((uint16_t)nb_texture, ftex);
		nb_texture = 0;
		Prm poly = {.primitive = obj->primitives};
		for (int i = 0; i < obj->primitives_len; i++) {
			printf("Write character[%d] @0x%x type %d\n", nb_texture, ftell(ftex), poly.primitive->type);
			switch (poly.primitive->type) {
				case PRM_TYPE_F3:
					poly.f3 += 1;
				break;
				case PRM_TYPE_F4:
					poly.f4 += 1;
				break;
				case PRM_TYPE_FT3:
					nb_texture++;
					write_16((uint16_t)poly.ft3->conv->id, ftex);
					write_16((uint16_t)poly.ft3->conv->width, ftex);
					printf("Size %dx%d\n", poly.ft3->conv->width,poly.ft3->conv->height);
					write_16((uint16_t)poly.ft3->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.ft3->conv->palette_id, ftex);
					write_16((uint16_t)poly.ft3->conv->length, ftex);
					for (int i=0; i<poly.ft3->conv->length; i++) {
						write_16((uint16_t)poly.ft3->conv->pixels[i], ftex);
					}
					poly.ft3 += 1;
				break;
				case PRM_TYPE_FT4:
					nb_texture++;
					write_16((uint16_t)poly.ft4->conv->id, ftex);
					write_16((uint16_t)poly.ft4->conv->width, ftex);
					write_16((uint16_t)poly.ft4->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.ft4->conv->palette_id, ftex);
					write_16((uint16_t)poly.ft4->conv->length, ftex);
					for (int i=0; i<poly.ft4->conv->length; i++) {
						write_16((uint16_t)poly.ft4->conv->pixels[i], ftex);
					}
					poly.ft4 += 1;
				break;
				case PRM_TYPE_G3:
					poly.g3 += 1;
				break;
				case PRM_TYPE_G4:
					poly.g4 += 1;
				break;
				case PRM_TYPE_GT3:
					nb_texture++;
					write_16((uint16_t)poly.gt3->conv->id, ftex);
					write_16((uint16_t)poly.gt3->conv->width, ftex);
					write_16((uint16_t)poly.gt3->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.gt3->conv->palette_id, ftex);
					write_16((uint16_t)poly.gt3->conv->length, ftex);
					for (int i=0; i<poly.gt3->conv->length; i++) {
						write_16((uint16_t)poly.gt3->conv->pixels[i], ftex);
					}
					poly.gt3 += 1;
				break;
				case PRM_TYPE_GT4:
					nb_texture++;
					write_16((uint16_t)poly.gt4->conv->id, ftex);
					write_16((uint16_t)poly.gt4->conv->width, ftex);
					write_16((uint16_t)poly.gt4->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.gt4->conv->palette_id, ftex);
					write_16((uint16_t)poly.gt4->conv->length, ftex);
					for (int i=0; i<poly.gt4->conv->length; i++) {
						write_16((uint16_t)poly.gt4->conv->pixels[i], ftex);
					}
					poly.gt4 += 1;
				break;
				case PRM_TYPE_LSF3:
					poly.lsf3 += 1;
				break;
				case PRM_TYPE_LSF4:
					poly.lsf4 += 1;
				break;
				case PRM_TYPE_LSFT3:
					nb_texture++;
					write_16((uint16_t)poly.lsft3->conv->id, ftex);
					write_16((uint16_t)poly.lsft3->conv->width, ftex);
					write_16((uint16_t)poly.lsft3->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsft3->conv->palette_id, ftex);
					write_16((uint16_t)poly.lsft3->conv->length, ftex);
					for (int i=0; i<poly.lsft3->conv->length; i++) {
						write_16((uint16_t)poly.lsft3->conv->pixels[i], ftex);
					}
					poly.lsft3 += 1;
				break;
				case PRM_TYPE_LSFT4:
					nb_texture++;
					write_16((uint16_t)poly.lsft4->conv->id, ftex);
					write_16((uint16_t)poly.lsft4->conv->width, ftex);
					write_16((uint16_t)poly.lsft4->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsft4->conv->palette_id, ftex);
					write_16((uint16_t)poly.lsft4->conv->length, ftex);
					for (int i=0; i<poly.lsft4->conv->length; i++) {
						write_16((uint16_t)poly.lsft4->conv->pixels[i], ftex);
					}
					poly.lsft4 += 1;
				break;
				case PRM_TYPE_LSG3:
					poly.lsg3 += 1;
				break;
				case PRM_TYPE_LSG4:
					poly.lsg4 += 1;
				break;
				case PRM_TYPE_LSGT3:
					nb_texture++;
					write_16((uint16_t)poly.lsgt3->conv->id, ftex);
					write_16((uint16_t)poly.lsgt3->conv->width, ftex);
					write_16((uint16_t)poly.lsgt3->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsgt3->conv->palette_id, ftex);
					write_16((uint16_t)poly.lsgt3->conv->length, ftex);
					for (int i=0; i<poly.lsgt3->conv->length; i++) {
						write_16((uint16_t)poly.lsgt3->conv->pixels[i], ftex);
					}
					poly.lsgt3 += 1;
				break;
				case PRM_TYPE_LSGT4:
					nb_texture++;
					write_16((uint16_t)poly.lsgt4->conv->id, ftex);
					write_16((uint16_t)poly.lsgt4->conv->width, ftex);
					write_16((uint16_t)poly.lsgt4->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.lsgt4->conv->palette_id, ftex);
					write_16((uint16_t)poly.lsgt4->conv->length, ftex);
					for (int i=0; i<poly.lsgt4->conv->length; i++) {
						write_16((uint16_t)poly.lsgt4->conv->pixels[i], ftex);
					}
					poly.lsgt4 += 1;
				break;
				case PRM_TYPE_TSPR:
				case PRM_TYPE_BSPR:
					nb_texture++;
					write_16((uint16_t)poly.spr->conv->id, ftex);
					write_16((uint16_t)poly.spr->conv->width, ftex);
					write_16((uint16_t)poly.spr->conv->height, ftex);
					printf("palette prim[%d] = %d\n", i, (uint16_t)poly.ft3->conv->palette_id);
					write_16((uint16_t)poly.spr->conv->palette_id, ftex);
					write_16((uint16_t)poly.spr->conv->length, ftex);
					for (int i=0; i<poly.spr->conv->length; i++) {
						write_16((uint16_t)poly.spr->conv->pixels[i], ftex);
					}
					poly.spr += 1;
				break;
				case PRM_TYPE_SPLINE:
					poly.spline += 1;
				break;
				case PRM_TYPE_POINT_LIGHT:
					poly.pointLight += 1;
				break;
				case PRM_TYPE_SPOT_LIGHT:
					poly.spotLight += 1;
				break;
				case PRM_TYPE_INFINITE_LIGHT:
					poly.infiniteLight += 1;
				break;
				default:
					die("bad primitive type\n");
			}
		}
	}

	fclose(fobj);
	fclose(ftex);
}


void object_draw(Object *object) {
	Prm poly = {.primitive = object->primitives};
	int primitives_len = object->primitives_len;

	vec3_t *vertex = object->vertices;

	// TODO: check for PRM_SINGLE_SIDED
	for (int i = 0; i < primitives_len; i++) {
		int coord0;
		int coord1;
		int coord2;
		int coord3;
    int coord[4];
		switch (poly.primitive->type) {
		case PRM_TYPE_GT3:
			coord0 = poly.gt3->coords[0];
			coord1 = poly.gt3->coords[1];
			coord2 = poly.gt3->coords[2];
			tris_t t0 = (tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.gt3->u2, poly.gt3->v2},
						.color = poly.gt3->color[2]
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.gt3->u1, poly.gt3->v1},
						.color = poly.gt3->color[1]
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.gt3->u0, poly.gt3->v0},
						.color = poly.gt3->color[0]
					},
				}
			};
			poly.gt3->conv = malloc(sizeof(render_texture_t));
      gl_generate_texture_from_tris(poly.gt3->conv, &t0, poly.gt3->texture);
			poly.gt3 += 1;
			break;

		case PRM_TYPE_GT4:
		printf("Gt4\n");
			coord0 = poly.gt4->coords[0];
			coord1 = poly.gt4->coords[1];
			coord2 = poly.gt4->coords[2];
			coord3 = poly.gt4->coords[3];
			quads_t q1 = {
				.vertices = {
					{
						.pos = vertex[coord0],
						.uv = {poly.gt4->u0, poly.gt4->v0},
						.color = poly.gt4->color[0]
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.gt4->u1, poly.gt4->v1},
						.color = poly.gt4->color[1]
					},
					{
						.pos = vertex[coord2],
						.uv = {poly.gt4->u2, poly.gt4->v2},
						.color = poly.gt4->color[2]
					},
					{
						.pos = vertex[coord3],
						.uv = {poly.gt4->u3, poly.gt4->v3},
						.color = poly.gt4->color[3]
					},
				}
			};
			poly.gt4->conv = malloc(sizeof(render_texture_t));
      gl_generate_texture_from_quad(poly.gt4->conv, &q1, poly.gt4->texture);

			poly.gt4 += 1;
			break;

		case PRM_TYPE_FT3:
			coord0 = poly.ft3->coords[0];
			coord1 = poly.ft3->coords[1];
			coord2 = poly.ft3->coords[2];
			tris_t t1 = (tris_t) {
				.vertices = {
					{
						.pos = vertex[coord2],
						.uv = {poly.ft3->u2, poly.ft3->v2},
						.color = poly.ft3->color
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.ft3->u1, poly.ft3->v1},
						.color = poly.ft3->color
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.ft3->u0, poly.ft3->v0},
						.color = poly.ft3->color
					},
				}
			};
			poly.ft3->conv = malloc(sizeof(render_texture_t));
			gl_generate_texture_from_tris(poly.ft3->conv, &t1, poly.ft3->texture);

			poly.ft3 += 1;
			break;

		case PRM_TYPE_FT4:
		printf("FT4\n");
			coord0 = poly.ft4->coords[0];
			coord1 = poly.ft4->coords[1];
			coord2 = poly.ft4->coords[2];
			coord3 = poly.ft4->coords[3];
			quads_t q2 = {
				.vertices = {
					{
						.pos = vertex[coord3],
						.uv = {poly.ft4->u3, poly.ft4->v3},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord2],
						.uv = {poly.ft4->u2, poly.ft4->v2},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord1],
						.uv = {poly.ft4->u1, poly.ft4->v1},
						.color = poly.ft4->color
					},
					{
						.pos = vertex[coord0],
						.uv = {poly.ft4->u0, poly.ft4->v0},
						.color = poly.ft4->color
					},
				}
			};
			poly.ft4->conv = malloc(sizeof(render_texture_t));
			gl_generate_texture_from_quad(poly.ft4->conv, &q2, poly.ft4->texture);

			poly.ft4 += 1;
			break;

		case PRM_TYPE_G3:
			poly.g3 += 1;
			break;

		case PRM_TYPE_G4:
			poly.g4 += 1;
			break;

		case PRM_TYPE_F3:
			poly.f3 += 1;
			break;

		case PRM_TYPE_F4:
			poly.f4 += 1;
			break;

		case PRM_TYPE_TSPR:
		case PRM_TYPE_BSPR:
			poly.spr += 1;
			break;

		default:
			break;

		}
	}
}
