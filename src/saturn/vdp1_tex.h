#ifndef VDP1_TEX_H
#define VDP1_TEX_H

#include "utils.h"

extern uint16_t* getVdp1VramAddress(uint16_t texture_index, uint8_t id, quads_t *q, rgba_t color, vec2i_t *size);
extern void reset_vdp1_pool(uint8_t id);
extern void init_vdp1_tex(void);
extern uint16_t canAllocateVdp1(uint16_t texture_index, uint8_t id, quads_t *quad);

#endif