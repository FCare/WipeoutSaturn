#ifndef __CONVERT_GL_H__
#define __CONVERT_GL_H__

extern int gl_init(void);
extern void gl_generate_texture_from_tris(render_texture_t *out, tris_t *t, int16_t texture);

#endif