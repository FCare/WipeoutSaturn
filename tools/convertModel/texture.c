#include <stdlib.h>
#include "texture.h"

#define TEXTURES_MAX 1024

static texture_t textures[TEXTURES_MAX];

static rgb1555_t palette[256];
static uint16_t palette_length;
static uint16_t textures_len = 0;

static texture_t *allocate_tex(uint32_t width, uint32_t height, rgba_t *buffer) {
    uint16_t texture_index = textures_len;
    textures[textures_len] = (texture_t){.width=width, .height=height, .pixels = buffer};
    texture_t *ret = &textures[textures_len];
    textures_len++;
    return ret;
}

static void updatePalette(rgb1555_t pix) {
	for (int i=0; i< 256; i++) {
		if (palette[i] == pix) return;
	}
	if (palette_length >= 256) {
		palette_length++;
	} else {
		palette[palette_length++] = pix;
	}
}

texture_t *texture_create(uint32_t width, uint32_t height, rgba_t *pixels){
  uint32_t byte_size = width * height * sizeof(rgb1555_t);
	return allocate_tex(width, height, pixels);
}