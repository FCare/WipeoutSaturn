#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "type.h"

typedef struct {
	uint16_t start;
	uint16_t len;
} texture_list_t;

typedef struct {
	vec2i_t size;
	rgb1555_t *pixels;
} render_texture_t;

uint16_t texture_create(uint32_t width, uint32_t height, rgba_t *pixels);

#endif