#include <yaul.h>
#include "render_vdp1.h"
#include "map.h"
#include "platform.h"
#include "vdp1_tex.h"
#include "tex.h"

static uint16_t nbCommand = 0;
static uint16_t gouraud_cmd = 0;

vdp1_cmdt_list_t *cmdt_list;
vdp1_cmdt_t *cmdts;
uint32_t cmdt_max;

static uint8_t id = 0;
static vdp1_vram_partitions_t _vdp1_vram_partitions;

static void cmdt_list_init(void)
{
  vec2i_t screen = platform_screen_size();
  int16_vec2_t size = {.x=screen.x, .y=screen.y};
  nbCommand = 0;

  cmdt_max = _vdp1_vram_partitions.cmdt_size/sizeof(vdp1_cmdt_t);
  cmdt_list = vdp1_cmdt_list_alloc(cmdt_max);
  assert(cmdt_list != NULL);

  (void)memset(&(cmdt_list->cmdts[0]), 0x00, sizeof(vdp1_cmdt_t) * (cmdt_max));

  const int16_vec2_t local_coords = INT16_VEC2_INITIALIZER(0,0);

  const int16_vec2_t system_clip_coords = size;

  /* Write directly to VDP1 VRAM. Since the first two command tables never
   * change, we can write directly */

  vdp1_cmdt_system_clip_coord_set(&(cmdt_list->cmdts[0]));
  vdp1_cmdt_vtx_system_clip_coord_set(&(cmdt_list->cmdts[0]), system_clip_coords);

  vdp1_cmdt_local_coord_set(&(cmdt_list->cmdts[1]));
  vdp1_cmdt_vtx_local_coord_set(&(cmdt_list->cmdts[1]), local_coords);

  vdp1_cmdt_end_set(&cmdt_list->cmdts[2]);

  cmdts = &(cmdt_list->cmdts[2]);
  cmdt_list->count = 3;

  vdp1_sync_cmdt_list_put(cmdt_list, 0);

}


void vdp1_init(void)
{
  vec2i_t screen = platform_screen_size();

  vdp1_vram_partitions_get(&_vdp1_vram_partitions);

  init_vdp1_tex();

  int16_vec2_t size = {.x=screen.x, .y=screen.y};
  const struct vdp1_env vdp1_env = {
          .bpp          = VDP1_ENV_BPP_16,
          .rotation     = VDP1_ENV_ROTATION_0,
          .color_mode   = VDP1_ENV_COLOR_MODE_RGB_PALETTE,
          .sprite_type  = 0,
          .erase_color  = RGB1555(0, 0, 0, 0),
          .erase_points = {
                  {0, 0 },
                  { size.x, size.y }
          }
  };
  vdp1_env_set(&vdp1_env);

  cmdt_list_init();

  vdp2_sprite_priority_set(0, 6);

  vdp1_sync_interval_set(0);
}

void render_vdp1_add(quads_t *quad, rgba_t color, uint16_t texture_index)
{
  printf(
    "vdp1 add %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)quad->vertices[0].pos.x,
    (int32_t)quad->vertices[0].pos.y,
    (int32_t)quad->vertices[1].pos.x,
    (int32_t)quad->vertices[1].pos.y,
    (int32_t)quad->vertices[2].pos.x,
    (int32_t)quad->vertices[2].pos.y,
    (int32_t)quad->vertices[3].pos.x,
    (int32_t)quad->vertices[3].pos.y
  );

  if (nbCommand >= cmdt_max-2) {
    printf("Too much command - Shall flush\n");
    render_vdp1_flush();
  }
  if (canAllocateVdp1(texture_index, id, quad) == 0) {
    printf("can not allocate - Shall flush\n");
    render_vdp1_flush();
  }

  const vdp1_cmdt_draw_mode_t draw_mode = {
          .color_mode           = VDP1_CMDT_CM_RGB_32768,
          .trans_pixel_disable  = true,
          .pre_clipping_disable = true,
          .end_code_disable     = false
  };
  const vdp1_cmdt_draw_mode_t draw_mode_gouraud = {
          .color_mode           = VDP1_CMDT_CM_RGB_32768,
          .trans_pixel_disable  = true,
          .pre_clipping_disable = true,
          .end_code_disable     = false,
          .cc_mode              = VDP1_CMDT_CC_GOURAUD
  };


  vdp1_cmdt_t *cmd = &cmdts[nbCommand];
  memset(cmd, 0x0, sizeof(vdp1_cmdt_t));

  vec2i_t size;
  uint16_t*character = getVdp1VramAddress(texture_index, id, quad, &size); //a revoir parce qu'il ne faut copier suivant le UV
  printf(
    "after %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)quad->vertices[0].pos.x,
    (int32_t)quad->vertices[0].pos.y,
    (int32_t)quad->vertices[1].pos.x,
    (int32_t)quad->vertices[1].pos.y,
    (int32_t)quad->vertices[2].pos.x,
    (int32_t)quad->vertices[2].pos.y,
    (int32_t)quad->vertices[3].pos.x,
    (int32_t)quad->vertices[3].pos.y
  );
  //Donc il faut transformer le UV en rectangle
  render_texture_t* src = get_tex(texture_index);
  vdp1_cmdt_end_clear(cmd);
  vdp1_cmdt_distorted_sprite_set(cmd); //Use distorted by default but it can be normal or scaled
  vdp1_cmdt_draw_mode_set(cmd, draw_mode);
  vdp1_cmdt_char_size_set(cmd, size.x, size.y);
  vdp1_cmdt_char_base_set(cmd, character);

  rgb1555_t gouraud = RGB1555(1, color.r>>3,  color.g>>3,  color.b>>3);
  if ((gouraud.r != 0x10) || (gouraud.g != 0x10) || (gouraud.b != 0x10)) {
    //apply gouraud
    vdp1_gouraud_table_t *gouraud_base;
    gouraud_base = &_vdp1_vram_partitions.gouraud_base[gouraud_cmd];
    gouraud_base->colors[0] = gouraud;
    gouraud_base->colors[1] = gouraud;
    gouraud_base->colors[2] = gouraud;
    gouraud_base->colors[3] = gouraud;
    gouraud_cmd++;
    vdp1_cmdt_draw_mode_set(cmd, draw_mode_gouraud);

   vdp1_cmdt_gouraud_base_set(cmd, (uint32_t)gouraud_base);
  }

  cmd->cmd_xa = (uint32_t)quad->vertices[0].pos.x;
  cmd->cmd_ya = (uint32_t)quad->vertices[0].pos.y;
  cmd->cmd_xb = (uint32_t)quad->vertices[1].pos.x;
  cmd->cmd_yb = (uint32_t)quad->vertices[1].pos.y;
  cmd->cmd_xc = (uint32_t)quad->vertices[2].pos.x;
  cmd->cmd_yc = (uint32_t)quad->vertices[2].pos.y;
  cmd->cmd_xd = (uint32_t)quad->vertices[3].pos.x;
  cmd->cmd_yd = (uint32_t)quad->vertices[3].pos.y;

  printf("####Add CMD########\n");

  nbCommand++;
}

void render_vdp1_flush(void) {
  //Flush shall not force the sync of the screen
  //Set the last command as end
  if (nbCommand > 0) vdp1_cmdt_end_set(&cmdts[nbCommand]);
  cmdt_list->count = nbCommand+3;
  printf("List count = %d\n", cmdt_list->count);
  vdp1_sync_cmdt_list_put(cmdt_list, 0);
  id = (id+1)%2;
  reset_vdp1_pool(id);
  nbCommand = 0;
  gouraud_cmd = 0;
  vdp1_sync_render();

  vdp1_sync();
  vdp1_sync_wait();
}

void render_vdp1_clear(void) {
  clear_vdp1_pool();
}