#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "texture.h"

#define TEXTURES_MAX 1024

static texture_t textures[TEXTURES_MAX];
static uint16_t textures_len = 0;

static rgb1555_t RGB888_RGB1555(uint8_t msb, uint8_t r, uint8_t g, uint8_t b) {
  return (rgb1555_t)(((msb&0x1)<<15) | ((r>>0x3)<<10) | ((g>>0x3)<<5) | (b>>0x3));
}

rgb1555_t convert_to_rgb(rgba_t val) {
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

texture_t *texture_create(uint32_t width, uint32_t height, rgba_t *pixels){
  uint32_t byte_size = width * height * sizeof(rgb1555_t);
  rgb1555_t palette[16] = {0};
  uint16_t nb_color = 0;
  for (int i=0; i<width*height; i++) {
    rgb1555_t color = convert_to_rgb(pixels[i]);
    int found = 0;
    for (int j=0; j<16; j++) {
      if (color == palette[j]) {
        found = 1;
        break;
      }
    }
    if (found==0){
      if (nb_color < 16) palette[nb_color] = color;
      nb_color++;
    }
  }
  // if (nb_color > 16) {
  //   LOGD("Palette too big %d\n", nb_color);
  //   exit(-1);
  // }
  texture_t *ret = allocate_tex(width, height, pixels);
  // if (nb_color > 256) {
  //   ret->format = COLOR_BANK_RGB;
  // }
  // if (nb_color > 128) {
  //   ret->format = COLOR_BANK_256_COL;
  //   ret->palette.pixels = malloc(256*sizeof(rgb1555_t));
  //   memcpy(ret->palette.pixels, &palette[0], 256*sizeof(rgb1555_t));
  // }
  // if (nb_color > 64) {
  //   ret->format = COLOR_BANK_128_COL;
  //   ret->palette = malloc(128*sizeof(rgb1555_t));
  //   memcpy(ret->palette, &palette[0], 128*sizeof(rgb1555_t));
  // }
  // if (nb_color > 16) {
  //   ret->format = COLOR_BANK_64_COL;
  //   ret->palette = malloc(64*sizeof(rgb1555_t));
  //   memcpy(ret->palette, &palette[0], 64*sizeof(rgb1555_t));
  if (nb_color > 16) {
    ret->format = COLOR_BANK_RGB;
  } else {
    //Only 4bpp lut is supported yet
    ret->format = LOOKUP_TABLE_16_COL;
    ret->palette.pixels = malloc(16*sizeof(rgb1555_t));
    memcpy(ret->palette.pixels, &palette[0], 16*sizeof(rgb1555_t));
    ret->palette.format = ret->format;
  }
	return ret;
}
