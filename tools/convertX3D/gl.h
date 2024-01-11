#ifndef __CONVERT_GL_H__
#define __CONVERT_GL_H__

#include "type.h"

extern int gl_init(step_func func, step_func final);
extern void gl_generate_texture_from_quad(render_texture_t *out, quads_t *t, texture_t *texture);

extern rgb1555_t rgb155_from_u8(uint8_t *v);


#endif