#include <stdlib.h>
#include "texture.h"

#define TEXTURES_MAX 1024

static texture_t textures[TEXTURES_MAX];

static rgb1555_t palette[256];
static uint16_t palette_length;
static uint16_t textures_len = 0;

static rgb1555_t RGB888_RGB1555(uint8_t msb, uint8_t r, uint8_t g, uint8_t b) {
  return (rgb1555_t)(((msb&0x1)<<15) | ((r>>0x3)<<10) | ((g>>0x3)<<5) | (b>>0x3));
}

static inline rgb1555_t convert_to_rgb(rgba_t val) {
  //RGB 16bits, MSB 1, transparent code 0
  if (val.a == 0) return RGB888_RGB1555(0,0,0,0);
  if ((val.b == 0) && (val.r == 0) && (val.g == 0)) {
    //Should be black but transparent usage makes it impossible
    return RGB888_RGB1555(1,0,0,1);
  }
  return RGB888_RGB1555(1, val.b, val.g, val.r);
}

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
  // rgb1555_t *buffer = (rgb1555_t *)malloc(byte_size);
  //
  // for (uint32_t i=0; i<width*height; i++) {
  //   buffer[i] = convert_to_rgb(pixels[i]);
	// 	// updatePalette(buffer[i]);
  // }

	return allocate_tex(width, height, pixels);
}