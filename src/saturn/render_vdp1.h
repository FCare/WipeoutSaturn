#ifndef __RENDER_VDP1_H__
#define __RENDER_VDP1_H__
#include "types.h"

extern void vdp1_init(void);
extern void render_vdp1_add(quads_t *quad, uint16_t texture_index);
extern void render_vdp1_flush(void);

#endif