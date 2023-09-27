#ifndef __RENDER_VDP2_H__
#define __RENDER_VDP2_H__

#define SCREEN_SIZE VDP2_TVMD_VERT_240
#define SCREEN_MODE VDP2_TVMD_HORZ_NORMAL_A

#include <yaul.h>
#include "types.h"

typedef enum {
  NBG0 = 0,
  LAYER_NB
} vdp2_layer_t;

extern void vdp2_init(void);
extern void vdp2_video_sync(void);
extern void render_vdp2_clear(void);

//need to add the priority here
void set_vdp2_texture(uint16_t texture_index, vec2i_t pos, vec2i_t size, vdp2_layer_t layer);

#endif