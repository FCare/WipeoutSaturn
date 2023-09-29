#include <yaul.h>
#include "render_vdp2.h"
#include "platform.h"
#include "tex.h"

typedef struct {
  uint16_t texture;
  uint8_t enable:1;
  uint8_t dirty:1;
  vec2i_t pos;
  vec2i_t size;
  vdp2_scrn_bitmap_format_t format;
  uint8_t priority;
} layer_ctrl_t;

static layer_ctrl_t layer_ctrl[LAYER_NB] = {
  {
    .enable = 0,
    .format = {
      .scroll_screen = VDP2_SCRN_NBG0,
      .ccc           = VDP2_SCRN_CCC_RGB_32768,
      .bitmap_size   = VDP2_SCRN_BITMAP_SIZE_512X256,
      .bitmap_base   = VDP2_VRAM_ADDR(0, 0x00000)
    },
    .priority = 1,
    .texture = 0xFFFF,
    .pos = vec2i(0,0),
    .size = vec2i(0,0),
  },
};

const vdp2_vram_cycp_t vram_cycp = {
        .pt[0].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[0].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

        .pt[1].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[1].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

        .pt[2].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[2].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0,

        .pt[3].t0 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t1 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t2 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t3 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t4 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t5 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t6 = VDP2_VRAM_CYCP_CHPNDR_NBG0,
        .pt[3].t7 = VDP2_VRAM_CYCP_CHPNDR_NBG0
};

static void _vblank_in_handler(void *work);

static void setup_vdp2(vdp2_layer_t layer) {
  //setup default layer state
  vdp2_scrn_bitmap_format_set(&layer_ctrl[layer].format);
  vdp2_scrn_priority_set(layer_ctrl[layer].format.scroll_screen, layer_ctrl[layer].priority);

  //Setup vram access
  vdp2_vram_cycp_set(&vram_cycp);
}

void set_vdp2_texture(uint16_t texture_index, vec2i_t pos, vec2i_t size, vdp2_layer_t layer) {
  if ((layer_ctrl[layer].texture != texture_index) ||
     (layer_ctrl[layer].pos.x != pos.x) ||
     (layer_ctrl[layer].pos.y != pos.y) ||
     (layer_ctrl[layer].size.x != size.x) ||
     (layer_ctrl[layer].size.y != size.y))
     {
       layer_ctrl[layer].texture = texture_index;
       layer_ctrl[layer].pos = pos;
       layer_ctrl[layer].size = size;
       layer_ctrl[layer].dirty = 1;
     }
  printf("Layer %d is used\n", layer);
  layer_ctrl[layer].enable = 1;
}

void render_vdp2_clear(void) {
  for (int layer = 0; layer< LAYER_NB; layer++) {
    layer_ctrl[layer].texture = 0xFFFF;
  }
}

static void updateLayerImage(uint16_t texture, vdp2_layer_t layer) {
  render_texture_t* src = get_tex(texture);
  volatile rgb1555_t *dst = layer_ctrl[layer].format.bitmap_base;
  for (int32_t i = 0; i< src->size.y; i++) {
    memcpy((void *)&(dst[512*i]), &src->pixels[i*src->size.x], src->size.x*sizeof(rgb1555_t));
  }
}

static void drawVdp2_splash(vdp2_layer_t layer) {
  uint16_t title_image = image_get_texture("wipeout/textures/wiptitle.tim");
  updateLayerImage(title_image, layer);
  layer_ctrl[layer].enable = 1;
  layer_ctrl[layer].dirty = 0;
}

static void vdp2_output_video() {
  vdp2_tvmd_display_res_set(VDP2_TVMD_INTERLACE_NONE, SCREEN_MODE,
    SCREEN_SIZE);
    vdp2_tvmd_display_set();
    vdp2_sync();
    vdp2_sync_wait();
}

void vdp2_init(void)
{
  LOGD("Setup vdp2\n");
  vdp_sync_vblank_in_set(_vblank_in_handler, NULL);
  setup_vdp2(NBG0);
  drawVdp2_splash(NBG0);
  vdp2_output_video();
}

void vdp2_video_sync(void) {
  //Wait VblankIn
  for (int i = 0; i< LAYER_NB; i++) {
    layer_ctrl[i].enable = 0;
  }
}

static void _vblank_in_handler(void *work __unused)
{
  //Do not update the layer if same texture is loaded unless a force (to be added as parameter)
  vdp2_scrn_disp_t val = vdp2_scrn_display_get();
  for (int i = 0; i< LAYER_NB; i++) {
    layer_ctrl_t* ctrl = &layer_ctrl[i];
    val &= ~ctrl->format.scroll_screen;
    if (ctrl->dirty != 0) {
      updateLayerImage(ctrl->texture, i);
      ctrl->dirty = 0;
    }
    if (ctrl->enable != 0) {
      val |= ctrl->format.scroll_screen;
    }
  }
  vdp2_scrn_display_set(val);
}
