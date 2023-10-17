#ifndef TEX_H
#define TEX_H

#include <yaul.h>
#include "utils.h"

typedef struct {
	vec2i_t size;
	rgb1555_t *pixels;
} render_texture_t;

extern void tex_reset(uint16_t len);
extern uint16_t allocate_tex(uint32_t width, uint32_t height, uint32_t size);
extern uint16_t create_sub_texture(uint32_t offset, uint32_t width, uint32_t height, uint16_t parent);
extern render_texture_t* get_tex(uint16_t texture);
extern uint16_t tex_length(void);
#endif