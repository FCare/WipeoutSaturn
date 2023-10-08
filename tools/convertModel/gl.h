#ifndef __CONVERT_GL_H__
#define __CONVERT_GL_H__

#include "type.h"

extern int gl_init(render_func func);
extern void gl_generate_texture_from_tris(render_texture_t *out, tris_t *t, texture_t *texture);

#endif