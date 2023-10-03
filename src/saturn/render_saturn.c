#include "render.h"
#include "render_vdp1.h"
#include "render_vdp2.h"
#include "map.h"
#include "tex.h"

#include "utils.h"
#include "mem.h"

#define NEAR_PLANE 128.0
#define FAR_PLANE (RENDER_FADEOUT_FAR)
#define TEXTURES_MAX 1024

uint16_t RENDER_NO_TEXTURE;

static uint8_t nb_planes = 0;

static vec2i_t screen_size;

static mat4_t view_mat = mat4_identity();
static mat4_t mvp_mat = mat4_identity();
static mat4_t projection_mat_2d = mat4_identity();
static mat4_t projection_mat_3d = mat4_identity();
static mat4_t sprite_mat = mat4_identity();

static void print_mat(mat4_t *m) {
  LOGD("\t[%d %d %d %d]\n\t[%d %d %d %d]\n\t[%d %d %d %d]\n\t[%d %d %d %d]\n",
  (m->arr[0]),
  (m->arr[1]),
  (m->arr[2]),
  (m->arr[3]),
  (m->arr[4]),
  (m->arr[5]),
  (m->arr[6]),
  (m->arr[7]),
  (m->arr[8]),
  (m->arr[9]),
  (m->arr[10]),
  (m->arr[11]),
  (m->arr[12]),
  (m->arr[13]),
  (m->arr[14]),
  (m->arr[15])
  );
}

void render_init(vec2i_t size) {
  screen_size = size;

  rgba_t white_pixels[64] = {
    rgba(128,128,128,255), rgba(128,128,128,255),
    rgba(128,128,128,255), rgba(128,128,128,255),
    rgba(128,128,128,255), rgba(128,128,128,255),
    rgba(128,128,128,255), rgba(128,128,128,255)
  };
  RENDER_NO_TEXTURE = render_texture_create(8, 1, white_pixels);

  render_set_screen_size(screen_size);
  vdp1_init();
  vdp2_init();

}
void render_cleanup(void){}

void render_set_screen_size(vec2i_t size){
  LOGD("%s\n", __FUNCTION__);
  LOGD("Screen %dx%d\n", screen_size.x, screen_size.y);
  //Screen is always same size on saturn port
  fix16_t lr = fix16_div(FIX16_ONE<<1, FIX16(screen_size.x));
  fix16_t bt = -fix16_div(FIX16_ONE<<1, FIX16(screen_size.y));
  projection_mat_2d = mat4(
    lr,  FIX16_ZERO,  FIX16_ZERO,  FIX16_ZERO,
    FIX16_ZERO,  bt,  FIX16_ZERO,  FIX16_ZERO,
    FIX16_ZERO,  FIX16_ZERO,  -FIX16_ONE,   FIX16_ZERO,
    -FIX16_ONE, FIX16_ONE, FIX16_ZERO, FIX16_ONE
  );
//Near = 128.0 FAR = 32000.0
projection_mat_3d = mat4(
  65523, FIX16_ZERO, FIX16_ZERO, FIX16_ZERO,
  FIX16_ZERO, 87365, FIX16_ZERO, FIX16_ZERO,
  FIX16_ZERO, FIX16_ZERO, -66062, -FIX16_ONE,
  FIX16_ZERO, FIX16_ZERO, -16844594, FIX16_ZERO
);
  LOGD("Proj 2D Mat= \n");
  print_mat(&projection_mat_2d);
  LOGD("Proj 3D Mat= \n");
  print_mat(&projection_mat_3d);
}
void render_set_resolution(render_resolution_t res){
  //Resolution is always same size on saturn port
}
void render_set_post_effect(render_post_effect_t post){}

vec2i_t render_size(void){
  return screen_size;
}

void render_frame_prepare(void){
  //Function called every frame need to prepare render list
  //Faire un clear screen a chaque frame (VDP1 VBlank Erase)?
  LOGD("%s\n", __FUNCTION__);
}
void render_frame_end(void){
  //On peut lancer la queue.
  LOGD("%s\n", __FUNCTION__);
  nb_planes = 0;
  render_vdp1_flush();
}

void render_set_view(vec3_t pos, vec3_t angles){
    LOGD("%s\n", __FUNCTION__);
  view_mat = mat4_identity();
  LOGD("View Mat= \n");
  print_mat(&view_mat);
  mat4_set_roll_pitch_yaw(&view_mat, vec3_fix16(angles.x, -angles.y + PLATFORM_PI, angles.z + PLATFORM_PI));
  LOGD("roll View Mat= \n");
  print_mat(&view_mat);
  mat4_translate(&view_mat, vec3_inv(pos));
  LOGD("translate View Mat= \n");
  print_mat(&view_mat);
  mat4_set_yaw_pitch_roll(&sprite_mat, vec3_fix16(-angles.x, angles.y - PLATFORM_PI, FIX16_ZERO));
  LOGD("yaw View Mat= \n");
  print_mat(&sprite_mat);

  LOGD("View Mat= \n");
  print_mat(&view_mat);
  LOGD("Proj 3D= \n");
  print_mat(&projection_mat_3d);
  mat4_mul(&mvp_mat, &projection_mat_3d, &view_mat);
  LOGD("MVP= \n");
  print_mat(&mvp_mat);
}
void render_set_view_2d(void){
  LOGD("%s\n", __FUNCTION__);
  view_mat = mat4_identity();
  LOGD("View Mat= \n");
  print_mat(&view_mat);
  LOGD("Proj 2D= \n");
  print_mat(&projection_mat_2d);
  mat4_mul(&mvp_mat, &projection_mat_2d, &view_mat);
  LOGD("MVP= \n");
  print_mat(&mvp_mat);
}
void render_set_model_mat(mat4_t *m){
    LOGD("%s\n", __FUNCTION__);
  mat4_t vm_mat;
  LOGD("M Mat= \n");
  print_mat(m);
  LOGD("View Mat= \n");
  print_mat(&view_mat);
	mat4_mul(&vm_mat, &view_mat, m);
  LOGD("Result= \n");
  print_mat(&vm_mat);
  LOGD("Proj 3D= \n");
  print_mat(&projection_mat_3d);
	mat4_mul(&mvp_mat, &projection_mat_3d, &vm_mat);
  LOGD("MVP= \n");
  print_mat(&mvp_mat);
}

void render_set_depth_write(bool enabled){}
void render_set_depth_test(bool enabled){}
void render_set_depth_offset(fix16_t offset){}
void render_set_screen_position(vec2_t pos){

}
void render_set_blend_mode(render_blend_mode_t mode){}
void render_set_cull_backface(bool enabled){}

vec3_t render_transform(vec3_t pos){
  return vec3_transform(vec3_transform(pos, &view_mat), &projection_mat_3d);
}

static void render_push_native_quads(quads_t *quad, rgba_t color, uint16_t texture_index) {
  fix16_t w2 = fix16_mul(FIX16(screen_size.x), FIX16(0.5));
  fix16_t h2 = fix16_mul(FIX16(screen_size.y), FIX16(0.5));
  nb_planes++;
  LOGD("MVP =\n");
  print_mat(&mvp_mat);
  for (int i = 0; i<4; i++) {
    // quad->vertices[i].pos.x += w2;
    // quad->vertices[i].pos.y += h2;
    LOGD("(%d,%d,%d)=>", quad->vertices[i].pos.x,quad->vertices[i].pos.y,quad->vertices[i].pos.z);
    quad->vertices[i].pos = vec3_transform(quad->vertices[i].pos, &mvp_mat);
    //Z-clampq
    LOGD("(%d,%d,%d)\n", quad->vertices[i].pos.x,quad->vertices[i].pos.y,quad->vertices[i].pos.z);
    if (quad->vertices[i].pos.z >= FIX16_ONE) {
      LOGD("discard due to Z=%d\n", (uint32_t)quad->vertices[i].pos.z);
      return;
    }
    quad->vertices[i].pos.x = fix16_mul(quad->vertices[i].pos.x, w2) + w2;
    quad->vertices[i].pos.y = h2 - fix16_mul(quad->vertices[i].pos.y, h2);
  }
  //Add a quad to the vdp1 list v0,v1,v2,v3
  render_vdp1_add(quad, color, texture_index);
}

void render_push_quads(quads_t *quad, uint16_t texture_index) {
  LOGD("%s\n", __FUNCTION__);
  render_push_native_quads(quad, rgba(128,128,128,255), texture_index);
}

void render_push_stripe(quads_t *quad, uint16_t texture_index) {
  LOGD("%s\n", __FUNCTION__);

  vec3_t temp = quad->vertices[1].pos;
  quad->vertices[1].pos = quad->vertices[2].pos;
  quad->vertices[2].pos = quad->vertices[3].pos;
  quad->vertices[3].pos = temp;

  quad->vertices[0].uv.x = 0;
  quad->vertices[0].uv.y = 0;
  quad->vertices[1].uv.x = 7;
  quad->vertices[1].uv.y = 0;
  quad->vertices[2].uv.x = 7;
  quad->vertices[2].uv.y = 1;
  quad->vertices[3].uv.x = 0;
  quad->vertices[3].uv.y = 1;

  render_push_native_quads(quad, quad->vertices[0].color, RENDER_NO_TEXTURE);
}

void render_push_tris(tris_t tris, uint16_t texture_index){
  LOGD("%s\n", __FUNCTION__);
  fix16_t w2 = fix16_mul(FIX16(screen_size.x), FIX16(0.5));
  fix16_t h2 = fix16_mul(FIX16(screen_size.y), FIX16(0.5));
  nb_planes++;
  LOGD("MVP =\n");
  print_mat(&mvp_mat);
  for (int i = 0; i<3; i++) {
    // quad->vertices[i].pos.x += w2;
    // quad->vertices[i].pos.y += h2;
    tris.vertices[i].pos = vec3_transform(tris.vertices[i].pos, &mvp_mat);
    //Z-clampq
    if (tris.vertices[i].pos.z >= FIX16_ONE) {
      LOGD("discard due to Z=%d\n", (uint32_t)tris.vertices[i].pos.z);
      return;
    }
    tris.vertices[i].pos.x = fix16_mul(tris.vertices[i].pos.x, w2) + w2;
    tris.vertices[i].pos.y = h2 - fix16_mul(tris.vertices[i].pos.y, h2);
  }

  quads_t q= (quads_t){
    .vertices = {
      tris.vertices[0], tris.vertices[1], tris.vertices[1], tris.vertices[2]
    }
  };

  q.vertices[0].uv.x = 0;
  q.vertices[0].uv.y = 0;
  q.vertices[1].uv.x = 7;
  q.vertices[1].uv.y = 0;
  q.vertices[2].uv.x = 7;
  q.vertices[2].uv.y = 1;
  q.vertices[3].uv.x = 0;
  q.vertices[3].uv.y = 1;

  //Add a quad to the vdp1 list v0,v1,v2,v3
  render_vdp1_add(&q,  rgba(128,128,128,255), RENDER_NO_TEXTURE);
}

void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture_index){
  LOGD("%s\n", __FUNCTION__);
  vec3_t p0 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5, -size.y * 0.5, 0), &sprite_mat));
  vec3_t p1 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5, -size.y * 0.5, 0), &sprite_mat));
  vec3_t p2 = vec3_add(pos, vec3_transform(vec3(-size.x * 0.5,  size.y * 0.5, 0), &sprite_mat));
  vec3_t p3 = vec3_add(pos, vec3_transform(vec3( size.x * 0.5,  size.y * 0.5, 0), &sprite_mat));

  render_texture_t *t = get_tex(texture_index);
  quads_t q = {
    .vertices = {
      {.pos = p0, .uv = {0, 0}, .color = color},
      {.pos = p1, .uv = {0 + t->size.x ,0}, .color = color},
      {.pos = p2, .uv = {0, 0 + t->size.y}, .color = color},
      {.pos = p3, .uv = {0 + t->size.x, 0 + t->size.y}, .color = color},
    }
  };
  render_push_native_quads(&q, color, texture_index);
}
void render_push_2d(vec2i_t pos, vec2i_t size, rgba_t color, uint16_t texture){
  LOGD("%s\n", __FUNCTION__);
  vec2i_t tex_size = render_texture_size(texture);
  //toujours des normals sprites ou du vdp2
  //If it is the first one, use a vdp2 surface to render
  //Can use NBG0 and NBG1, with Sprite in between based on priority (to be optimized later)
  LOGD("Size = %dx%d Texture=%dx%d\n", size.x, size.y, tex_size.x, tex_size.y);
  if ((nb_planes == 0) && (size.x == tex_size.x) && (size.y == tex_size.y)) {
    nb_planes++;
    set_vdp2_texture(texture, pos, size, NBG0);
  } else {
    render_push_2d_tile(pos, vec2i(0, 0), tex_size, size, color, texture);
  }
}
void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size, vec2i_t size, rgba_t color, uint16_t texture_index){
  LOGD("%s\n", __FUNCTION__);
  //toujours des scaled_sprites
  vec2_t p = vec2(pos.x, pos.y);
  vec2_t s = vec2(size.x, size.y);
  quads_t q = (quads_t){
		.vertices = {
      {
        .pos = {.x=p.x, .y=p.y, .z=FIX16_ZERO},
        .uv = {.x=uv_offset.x , .y=uv_offset.y},
        .color = color
      },
      {
        .pos = {.x=p.x + s.x, .y=p.y, .z=FIX16_ZERO},
        .uv = {.x=uv_offset.x +  uv_size.x, .y=uv_offset.y},
        .color = color
      },
      {
        .pos = {.x=p.x + s.x, .y=p.y + s.y, .z=FIX16_ZERO},
        .uv = {.x=uv_offset.x + uv_size.x, .y=uv_offset.y + uv_size.y},
        .color = color
      },
      {
        .pos = {.x=p.x, .y=p.y + s.y, .z=FIX16_ZERO},
        .uv = {.x=uv_offset.x , .y=uv_offset.y + uv_size.y},
        .color = color
      },
		}
	};
  LOGD(
    " pos %dx%d %dx%d %dx%d %dx%d\n",
    fix16_int32_to(q.vertices[0].pos.x),
    fix16_int32_to(q.vertices[0].pos.y),
    fix16_int32_to(q.vertices[1].pos.x),
    fix16_int32_to(q.vertices[1].pos.y),
    fix16_int32_to(q.vertices[2].pos.x),
    fix16_int32_to(q.vertices[2].pos.y),
    fix16_int32_to(q.vertices[3].pos.x),
    fix16_int32_to(q.vertices[3].pos.y)
  );
  LOGD(
    " uv %dx%d %dx%d %dx%d %dx%d\n",
    (uint32_t)q.vertices[0].uv.x,
    (uint32_t)q.vertices[0].uv.y,
    (uint32_t)q.vertices[1].uv.x,
    (uint32_t)q.vertices[1].uv.y,
    (uint32_t)q.vertices[2].uv.x,
    (uint32_t)q.vertices[2].uv.y,
    (uint32_t)q.vertices[3].uv.x,
    (uint32_t)q.vertices[3].uv.y
  );
  render_push_native_quads(&q, color, texture_index);
}

static inline rgb1555_t convert_to_rgb(rgba_t val) {
  //RGB 16bits, MSB 1, transparent code 0
  if (val.a == 0) return RGB888_RGB1555(0,0,0,0);
  if ((val.b == 0) && (val.r == 0) && (val.g == 0)) {
    //Should be black but transparent usage makes it impossible
    return RGB888_RGB1555(1,0,0,1);
  }
  return RGB888_RGB1555(1, val.b, val.g, val.r);
}

uint16_t render_texture_create(uint32_t width, uint32_t height, rgba_t *pixels){
  uint32_t byte_size = width * height * sizeof(rgb1555_t);
  if (byte_size < 4096)
    LOGD("Need %d B\n", byte_size);
  else
    LOGD("Need %d kB\n", byte_size/1024);
  LOGD("Create Texture(%dx%d)\n", width, height);
  rgb1555_t *buffer = (rgb1555_t *)tex_bump(byte_size);

  for (uint32_t i=0; i<width*height; i++) {
    buffer[i] = convert_to_rgb(pixels[i]);
  }

	return allocate_tex(width, height, buffer);
}

uint16_t render_texture_create_555(uint32_t width, uint32_t height, rgb1555_t *pixels){
    return allocate_tex(width, height, pixels);
}
vec2i_t render_texture_size(uint16_t texture){
  LOGD("%s\n", __FUNCTION__);
  return get_tex(texture)->size;
}
void render_texture_replace_pixels(uint16_t texture_index, rgba_t *pixels){
  LOGD("%s\n", __FUNCTION__);
  render_texture_t *t = get_tex(texture_index);
  for (int i=0; i<t->size.x*t->size.y; i++) {
    t->pixels[i] = convert_to_rgb(pixels[i]);
  }
}
uint16_t render_textures_len(void){
  return tex_length();
}
void render_textures_reset(uint16_t len){
  LOGD("%s %d\n", __FUNCTION__ , len);
  tex_reset(len);
  render_vdp1_clear();
  render_vdp2_clear();
}
void render_textures_dump(const char *path){LOGD("%s\n", __FUNCTION__); }