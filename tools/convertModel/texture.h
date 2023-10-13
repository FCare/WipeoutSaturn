#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "type.h"

typedef enum {
	COLOR_BANK_16_COL = 0,
	LOOKUP_TABLE_16_COL = 1,
	COLOR_BANK_64_COL = 2,
	COLOR_BANK_128_COL = 3,
	COLOR_BANK_256_COL = 4,
	COLOR_BANK_RGB = 5,
	COLOR_MAX
} color_mode_t;

typedef struct {
	color_mode_t format;
	rgb1555_t* pixels;
	uint16_t index_in_file;
} palette_t;

typedef struct {
	color_mode_t format;
	palette_t palette;
	int16_t width;
  int16_t height;
	rgba_t *pixels;
	int16_t id;
} texture_t;

typedef struct {
	uint16_t len;
  texture_t **texture;
} texture_list_t;

typedef struct {
	int16_t id;
	int16_t palette_id;
	int16_t width;
  int16_t height;
	int16_t length;
	rgb1555_t *pixels;
} render_texture_t;

texture_t *texture_create(uint32_t width, uint32_t height, rgba_t *pixels);
rgb1555_t convert_to_rgb(rgba_t val);

#endif