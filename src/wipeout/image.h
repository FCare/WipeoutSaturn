#ifndef INIT_H
#define INIT_H

#include "../types.h"

#define COLOR_BANK_16_COL 	VDP1_CMDT_CM_CB_16
#define LOOKUP_TABLE_16_COL VDP1_CMDT_CM_CLUT_16
#define COLOR_BANK_64_COL 	VDP1_CMDT_CM_CB_64
#define COLOR_BANK_128_COL 	VDP1_CMDT_CM_CB_128
#define COLOR_BANK_256_COL 	VDP1_CMDT_CM_CB_256
#define COLOR_BANK_RGB 			VDP1_CMDT_CM_RGB_32768
typedef uint16_t color_mode_t;

typedef struct {
	uint16_t start;
	uint16_t len;
} texture_list_t;

#define texture_list_empty() ((texture_list_t){0, 0})

typedef struct {
	uint32_t width;
	uint32_t height;
	rgba_t *pixels;
} image_t;

typedef struct {
	uint16_t width;
	uint16_t stride;
	uint16_t height;
	uint16_t offset;
} font_character_t;

typedef struct {
	uint16_t format;
	uint16_t nbQuads;
	font_character_t character[];
} saturn_font_t;

typedef struct {
	color_mode_t format;
	uint16_t texture;
	uint16_t width;
	rgb1555_t *pixels;
} palette_t;

typedef struct {
	union{
		uint16_t id;
		uint16_t texture;
	};
	uint16_t width;
	uint16_t height;
	uint16_t palette_id;
	uint16_t length;
	rgb1555_t *pixels;
} character_t;

typedef struct {
	uint16_t nb_characters;
	character_t **character;
} character_list_t;

typedef struct {
	uint16_t nb_palettes;
	palette_t **pal;
	uint16_t nb_objects;
	character_list_t *characters;
} saturn_image_ctrl_t;

typedef struct {
	uint32_t len;
	uint8_t *entries[];
} cmp_t;

image_t *image_alloc(uint32_t width, uint32_t height);
void image_copy(image_t *src, image_t *dst, uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy);
image_t *image_load_from_bytes(uint8_t *bytes, bool transparent);

cmp_t *image_load_compressed(char *name);

uint16_t image_get_texture(char *name);
uint16_t image_get_texture_semi_trans(char *name);
texture_list_t image_get_compressed_textures(char *name);
uint16_t texture_from_list(texture_list_t tl, uint16_t index);

saturn_image_ctrl_t* image_get_saturn_textures(char *name);

#endif