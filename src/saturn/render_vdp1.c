#include <yaul.h>
#include "render_vdp1.h"
#include "map.h"
#include "platform.h"
#include "vdp1_tex.h"
#include "tex.h"
#include "mem.h"
#include "utils.h"

static uint16_t nbCommand = 0;
static uint16_t gouraud_cmd = 0;

vdp1_cmdt_list_t *cmdt_list;
vdp1_cmdt_t *cmdts;
uint32_t cmdt_max;

chain_t *chain;

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

  chain = (chain_t*)mem_bump(cmdt_max*sizeof(chain_t));

  /* Write directly to VDP1 VRAM. Since the first two command tables never
   * change, we can write directly */
   const int16_vec2_t system_clip_coords = size;
   const int16_vec2_t local_coords = INT16_VEC2_INITIALIZER(size.x>>1,size.y>>1);

  vdp1_cmdt_system_clip_coord_set(&(cmdt_list->cmdts[0]));
  vdp1_cmdt_vtx_system_clip_coord_set(&(cmdt_list->cmdts[0]), system_clip_coords);

  vdp1_cmdt_local_coord_set(&(cmdt_list->cmdts[1]));
  vdp1_cmdt_vtx_local_coord_set(&(cmdt_list->cmdts[1]), local_coords);
  vdp1_cmdt_link_type_set(&(cmdt_list->cmdts[1]), VDP1_CMDT_LINK_TYPE_JUMP_ASSIGN);
  cmdt_list->cmdts[1].cmd_link = (uint16_t)((uint32_t)&cmdt_list->cmdts[2]-(uint32_t)&cmdt_list->cmdts[0])>>3;

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

void render_vdp1_add_saturn(quads_saturn_t *quad, uint16_t texture_index, Object_Saturn *object __unused, int useMesh){
  vdp1_vram_t character = 0;
  vec2i_t size = vec2i(0,0);
  LOGD(
    "vdp1 add saturn %dx%d %dx%d %dx%d %dx%d\n",
    fix16_int32_to(quad->vertices[0].pos.x),
    fix16_int32_to(quad->vertices[0].pos.y),
    fix16_int32_to(quad->vertices[1].pos.x),
    fix16_int32_to(quad->vertices[1].pos.y),
    fix16_int32_to(quad->vertices[2].pos.x),
    fix16_int32_to(quad->vertices[2].pos.y),
    fix16_int32_to(quad->vertices[3].pos.x),
    fix16_int32_to(quad->vertices[3].pos.y)
  );

  assert (nbCommand < cmdt_max-2);
  // assert(canAllocateVdp1_saturn(texture_index, id, quad) != 0);

  vdp1_cmdt_t *cmd = &cmdts[nbCommand];
  memset(cmd, 0x0, sizeof(vdp1_cmdt_t));

  chain[nbCommand].z = 0;
  for (int i = 0; i<4; i++) {
    chain[nbCommand].z += quad->vertices[i].pos.z;
  }
  chain[nbCommand].z >>= 2;


  LOGD(
    "after %dx%d %dx%d %dx%d %dx%d\n",
    fix16_int32_to(quad->vertices[0].pos.x),
    fix16_int32_to(quad->vertices[0].pos.y),
    fix16_int32_to(quad->vertices[1].pos.x),
    fix16_int32_to(quad->vertices[1].pos.y),
    fix16_int32_to(quad->vertices[2].pos.x),
    fix16_int32_to(quad->vertices[2].pos.y),
    fix16_int32_to(quad->vertices[3].pos.x),
    fix16_int32_to(quad->vertices[3].pos.y)
  );
  vdp1_cmdt_end_clear(cmd);

  vdp1_cmdt_draw_mode_t draw_mode = {
    .trans_pixel_disable  = true,
    .pre_clipping_disable = true,
    .end_code_disable     = true,
  };

  if (texture_index == RENDER_NO_TEXTURE_SATURN) {
    LOGD("No color, using a white polygon\n");
    vdp1_cmdt_polygon_set(cmd);
    vdp1_cmdt_color_set(cmd, quad->color);
  } else {
    vdp1_texture_t *tex = get_vdp1_texture(texture_index);
    character = (vdp1_vram_t)tex->pixels;
    size = tex->size;
    if (object->palette_id == 0xFFFF) {
      object->palette_id = allocate_vdp1_texture((void*)object->palette, 16,1, 16);
    }
    vdp1_texture_t *pal = get_vdp1_texture(object->palette_id);
    //All vdp1 texture are 4bpp LUT
    vdp1_cmdt_color_mode1_set(cmd, (uint32_t)pal->pixels);
    draw_mode.color_mode    = LOOKUP_TABLE_16_COL;
    vdp1_cmdt_distorted_sprite_set(cmd); //Use distorted by default but it can be normal or scaled
  }

  if (useMesh != 0) {
    draw_mode.mesh_enable = 1;
  }
  vdp1_cmdt_draw_mode_set(cmd, draw_mode);
  vdp1_cmdt_char_size_set(cmd, size.x, size.y);
  vdp1_cmdt_char_base_set(cmd, character);
  vdp1_cmdt_link_type_set(cmd, VDP1_CMDT_LINK_TYPE_JUMP_ASSIGN);
  cmd->cmd_link = (uint16_t)((uint32_t)&cmdts[nbCommand+1]-(uint32_t)&cmdt_list->cmdts[0])>>3;
  chain[nbCommand].id = nbCommand;

  uint8_t needGouraud = quad->useLight;
  if (needGouraud != 0) {
    //apply gouraud
    //Shall not be shared between lists
    vdp1_gouraud_table_t *gouraud_base;
    gouraud_base = &_vdp1_vram_partitions.gouraud_base[gouraud_cmd];
    gouraud_base->colors[0] = quad->vertices[0].light;
    gouraud_base->colors[1] = quad->vertices[1].light;
    gouraud_base->colors[2] = quad->vertices[2].light;
    gouraud_base->colors[3] = quad->vertices[3].light;
    gouraud_cmd++;
    draw_mode.cc_mode = VDP1_CMDT_CC_GOURAUD;

    vdp1_cmdt_gouraud_base_set(cmd, (uint32_t)gouraud_base);
  }
  vdp1_cmdt_draw_mode_set(cmd, draw_mode);

  cmd->cmd_xa = fix16_int32_to(quad->vertices[0].pos.x);
  cmd->cmd_ya = fix16_int32_to(quad->vertices[0].pos.y);
  cmd->cmd_xb = fix16_int32_to(quad->vertices[1].pos.x);
  cmd->cmd_yb = fix16_int32_to(quad->vertices[1].pos.y);
  cmd->cmd_xc = fix16_int32_to(quad->vertices[2].pos.x);
  cmd->cmd_yc = fix16_int32_to(quad->vertices[2].pos.y);
  cmd->cmd_xd = fix16_int32_to(quad->vertices[3].pos.x);
  cmd->cmd_yd = fix16_int32_to(quad->vertices[3].pos.y);
  LOGD("####Add CMD########\n");

  nbCommand++;
}

void render_vdp1_add(quads_t *quad, rgba_t color, uint16_t texture_index)
{
  LOGD(
    "vdp1 add %dx%d %dx%d %dx%d %dx%d\n",
    fix16_int32_to(quad->vertices[0].pos.x),
    fix16_int32_to(quad->vertices[0].pos.y),
    fix16_int32_to(quad->vertices[1].pos.x),
    fix16_int32_to(quad->vertices[1].pos.y),
    fix16_int32_to(quad->vertices[2].pos.x),
    fix16_int32_to(quad->vertices[2].pos.y),
    fix16_int32_to(quad->vertices[3].pos.x),
    fix16_int32_to(quad->vertices[3].pos.y)
  );

  assert (nbCommand < cmdt_max-2);
  assert(canAllocateVdp1(texture_index, quad) != 0);

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

  chain[nbCommand].z = 0;
  for (int i = 0; i<4; i++) {
    chain[nbCommand].z += quad->vertices[i].pos.z;
  }
  chain[nbCommand].z >>= 2;

  vec2i_t size = vec2i(0,0);
  uint16_t*character = getVdp1VramAddress(texture_index, quad, &size); //a revoir parce qu'il ne faut copier suivant le UV
  LOGD(
    "after %dx%d %dx%d %dx%d %dx%d\n",
    fix16_int32_to(quad->vertices[0].pos.x),
    fix16_int32_to(quad->vertices[0].pos.y),
    fix16_int32_to(quad->vertices[1].pos.x),
    fix16_int32_to(quad->vertices[1].pos.y),
    fix16_int32_to(quad->vertices[2].pos.x),
    fix16_int32_to(quad->vertices[2].pos.y),
    fix16_int32_to(quad->vertices[3].pos.x),
    fix16_int32_to(quad->vertices[3].pos.y)
  );
  //Donc il faut transformer le UV en rectangle
  vdp1_cmdt_end_clear(cmd);
  vdp1_cmdt_distorted_sprite_set(cmd); //Use distorted by default but it can be normal or scaled
  vdp1_cmdt_draw_mode_set(cmd, draw_mode);
  vdp1_cmdt_char_size_set(cmd, size.x, size.y);
  vdp1_cmdt_char_base_set(cmd, (vdp1_vram_t)character);
  vdp1_cmdt_link_type_set(cmd, VDP1_CMDT_LINK_TYPE_JUMP_ASSIGN);
  cmd->cmd_link = (uint16_t)((uint32_t)&cmdts[nbCommand+1]-(uint32_t)&cmdt_list->cmdts[0])>>3;
  chain[nbCommand].id = nbCommand;

  rgb1555_t gouraud = RGB1555(1, color.r>>3,  color.g>>3,  color.b>>3);
  if ((gouraud.r != 0x10) || (gouraud.g != 0x10) || (gouraud.b != 0x10)) {
    //apply gouraud
    //Shall not be shared between lists
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

  cmd->cmd_xa = fix16_int32_to(quad->vertices[0].pos.x);
  cmd->cmd_ya = fix16_int32_to(quad->vertices[0].pos.y);
  cmd->cmd_xb = fix16_int32_to(quad->vertices[1].pos.x);
  cmd->cmd_yb = fix16_int32_to(quad->vertices[1].pos.y);
  cmd->cmd_xc = fix16_int32_to(quad->vertices[2].pos.x);
  cmd->cmd_yc = fix16_int32_to(quad->vertices[2].pos.y);
  cmd->cmd_xd = fix16_int32_to(quad->vertices[3].pos.x);
  cmd->cmd_yd = fix16_int32_to(quad->vertices[3].pos.y);

  LOGD("####Add CMD########\n");

  nbCommand++;
}

void render_vdp1_flush(void) {
  //Flush shall not force the sync of the screen
  if (nbCommand > 0) {
    //sort all the quads
    //Set the last command as end
    quickSort_Z(&chain[0], 0, nbCommand-1);
    //Reordering jmp adress based on order
    cmdt_list->cmdts[1].cmd_link = (chain[0].id+2)<<2;
    for (int i = 0; i< nbCommand-1; i++) {
      cmdts[chain[i].id].cmd_link = (chain[i+1].id+2)<<2;
    }
    cmdts[chain[nbCommand-1].id].cmd_link = (nbCommand+2)<<2;
    vdp1_cmdt_end_set(&cmdts[nbCommand]);
  }
  cmdt_list->count = nbCommand+3;

  LOGD("List count = %d\n", cmdt_list->count);
  vdp1_sync_cmdt_list_put(cmdt_list, 0);
  id = (id+1)%2;
  reset_vdp1_pool(id);
  nbCommand = 0;
  gouraud_cmd = id*(VDP1_VRAM_DEFAULT_GOURAUD_COUNT>>1);
  vdp1_sync_render();
}

void render_vdp1_clear(void) {
  clear_vdp1_pool();
}