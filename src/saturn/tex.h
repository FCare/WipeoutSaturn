#ifndef TEX_H
#define TEX_H

#include <yaul.h>
#include "utils.h"

typedef struct {
	vec2i_t size;
	rgb1555_t *pixels;
} render_texture_t;

extern void *tex_bump(uint32_t size);
extern void tex_reset(uint16_t len);
extern uint16_t allocate_tex(uint32_t width, uint32_t height, rgb1555_t *buffer);
extern render_texture_t* get_tex(uint16_t texture);
extern uint16_t tex_length(void);
#endif