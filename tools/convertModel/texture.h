#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "type.h"

typedef struct {
	int16_t width;
  int16_t height;
	rgba_t *pixels;
} texture_t;

typedef struct {
	uint16_t len;
  texture_t **texture;
} texture_list_t;

typedef struct {
	int16_t width;
  int16_t height;
	rgb1555_t *pixels;
} render_texture_t;

texture_t *texture_create(uint32_t width, uint32_t height, rgba_t *pixels);

#endif