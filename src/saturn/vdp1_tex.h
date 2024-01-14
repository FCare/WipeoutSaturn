#ifndef VDP1_TEX_H
#define VDP1_TEX_H

#include "utils.h"

typedef struct {
	vec2i_t size;
  uint16_t index;
	vec2_t uv[4];
	rgb1555_t *pixels;
	uint8_t used;
} vdp1_texture_t;

extern uint16_t* getVdp1VramAddress(uint16_t texture_index, quads_t *q, vec2i_t *size);
extern uint16_t* getVdp1VramAddress_Saturn(uint16_t texture_index);
extern void reset_vdp1_pool();
extern void clear_vdp1_pool(void);
extern void init_vdp1_tex(void);
extern uint16_t canAllocateVdp1(uint16_t texture_index, quads_t *quad);

//new API
extern uint16_t allocate_vdp1_texture(void* pixel, uint16_t w, uint16_t h, uint8_t elt_size);
extern vdp1_texture_t* get_vdp1_texture(uint16_t texture_index);

#endif