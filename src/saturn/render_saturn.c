#include "render.h"
#include "render_vdp1.h"
#include "render_vdp2.h"
#include "map.h"
#include "tex.h"

#include "utils.h"
#include "mem.h"

#define NEAR_PLANE 16.0
#define FAR_PLANE (RENDER_FADEOUT_FAR)
#define TEXTURES_MAX 1024

uint16_t RENDER_NO_TEXTURE;

static uint8_t nb_planes = 0;

static vec2i_t screen_size;

static mat4_t view_mat = mat4_identity();
static mat4_t mvp_mat = mat4_identity();
static mat4_t projection_mat = mat4_identity();
static mat4_t sprite_mat = mat4_identity();

static void print_mat(mat4_t *m) {
  printf("[%d %d %d %d]\n      [%d %d %d %d]\n      [%d %d %d %d]\n      [%d %d %d %d]\n",
  (int32_t)(m->m[0]*1000.0),
  (int32_t)(m->m[1]*1000.0),
  (int32_t)(m->m[2]*1000.0),
  (int32_t)(m->m[3]*1000.0),
  (int32_t)(m->m[4]*1000.0),
  (int32_t)(m->m[5]*1000.0),
  (int32_t)(m->m[6]*1000.0),
  (int32_t)(m->m[7]*1000.0),
  (int32_t)(m->m[8]*1000.0),
  (int32_t)(m->m[9]*1000.0),
  (int32_t)(m->m[10]*1000.0),
  (int32_t)(m->m[11]*1000.0),
  (int32_t)(m->m[12]*1000.0),
  (int32_t)(m->m[13]*1000.0),
  (int32_t)(m->m[14]*1000.0),
  (int32_t)(m->m[15]*1000.0)
  );
}

void render_init(vec2i_t size) {
  screen_size = size;

  rgba_t white_pixels[4] = {
    rgba(128,128,128,255), rgba(128,128,128,255),
    rgba(128,128,128,255), rgba(128,128,128,255)
  };
  RENDER_NO_TEXTURE = render_texture_create(2, 2, white_pixels);

  vdp1_init();
  vdp2_init();

}
void render_cleanup(void){}

void render_set_screen_size(vec2i_t size){
  //Screen is always same size on saturn port
  float aspect = (float)screen_size.x / (float)screen_size.y;
  float fov = (73.75 / 180.0) * 3.14159265358;
  float f = 1.0 / tan(fov / 2);
  float nf = 1.0 / (NEAR_PLANE - FAR_PLANE);
  projection_mat = mat4(
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (FAR_PLANE + NEAR_PLANE) * nf, -1,
    0, 0, 2 * FAR_PLANE * NEAR_PLANE * nf, 0
  );
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
  printf("%s\n", __FUNCTION__);
}
void render_frame_end(void){
  //On peut lancer la queue.
  printf("%s\n", __FUNCTION__);
  nb_planes = 0;
  render_vdp1_flush();
}

void render_set_view(vec3_t pos, vec3_t angles){
    printf("%s\n", __FUNCTION__);
  view_mat = mat4_identity();
  mat4_set_translation(&view_mat, vec3(0, 0, 0));
  mat4_set_roll_pitch_yaw(&view_mat, vec3(angles.x, -angles.y + M_PI, angles.z + M_PI));
  mat4_translate(&view_mat, vec3_inv(pos));
  mat4_set_yaw_pitch_roll(&sprite_mat, vec3(-angles.x, angles.y - M_PI, 0));

  render_set_model_mat(&mat4_identity());
}
void render_set_view_2d(void){
  printf("%s\n", __FUNCTION__);

  float near = -1.0f;
  float far = 1.0f;
  float left = 0.0f;
  float right = (float)screen_size.x;
  float bottom = (float)screen_size.y;
  float top = 0.0f;
  float lr = 1.0f / (left - right);
  float bt = 1.0f / (bottom - top);
  float nf = 1.0f / (near - far);
  mvp_mat = mat4(
    -2.0f * lr,  0.0f,  0.0f,  0.0f,
    0.0f,  -2.0f * bt,  0.0f,  0.0f,
    0.0f,        0.0f,  2.0f * nf,    0.0f,
    (left + right) * lr, (top + bottom) * bt, (far + near) * nf, 1.0f
  );

}
void render_set_model_mat(mat4_t *m){
    printf("%s\n", __FUNCTION__);
  mat4_t vm_mat;
	mat4_mul(&vm_mat, &view_mat, m);
	mat4_mul(&mvp_mat, &projection_mat, &vm_mat);
}
void render_set_depth_write(bool enabled){}
void render_set_depth_test(bool enabled){}
void render_set_depth_offset(float offset){}
void render_set_screen_position(vec2_t pos){}
void render_set_blend_mode(render_blend_mode_t mode){}
void render_set_cull_backface(bool enabled){}

vec3_t render_transform(vec3_t pos){
  return vec3_transform(vec3_transform(pos, &view_mat), &projection_mat);
}

static void render_push_native_quads(quads_t *quad, rgba_t color, uint16_t texture_index) {
  float w2 = screen_size.x * 0.5;
  float h2 = screen_size.y * 0.5;
  nb_planes++;
  for (int i = 0; i<4; i++) {
    // quad->vertices[i].pos.x += w2;
    // quad->vertices[i].pos.y += h2;

    quad->vertices[i].pos = vec3_transform(quad->vertices[i].pos, &mvp_mat);
    //Z-clamp
    if (quad->vertices[i].pos.z >= 1.0) {
      printf("discard due to Z=%d\n", (uint32_t)quad->vertices[i].pos.z);
      return;
    }
    quad->vertices[i].pos.x = quad->vertices[i].pos.x * w2 + w2;
    quad->vertices[i].pos.y = h2 - quad->vertices[i].pos.y * h2;
  }
  //Add a quad to the vdp1 list v0,v1,v2,v3
  render_vdp1_add(quad, color, texture_index);
}

void render_push_quads(quads_t *quad, uint16_t texture_index) {
  printf("%s\n", __FUNCTION__);
  render_push_native_quads(quad, rgba(128,128,128,255), texture_index);
}

void render_push_stripe(quads_t *quad, uint16_t texture_index) {
  printf("%s\n", __FUNCTION__);
  for (int i = 0; i<4; i++) {
    quad->vertices[i].pos = vec3_transform(quad->vertices[i].pos, &mvp_mat);
    if (quad->vertices[i].pos.z >= 1.0) return;
  }
  vec3_t temp = quad->vertices[1].pos;
  quad->vertices[1].pos = quad->vertices[2].pos;
  quad->vertices[2].pos = quad->vertices[3].pos;
  quad->vertices[3].pos = temp;
  //Add a quad to the vdp1 list v0,v2,v3,v1
  // render_vdp1_add(quad, rgba(128,128,128,255), texture_index);
}

void render_push_tris(tris_t tris, uint16_t texture_index){
  printf("%s\n", __FUNCTION__);
  printf(
    " pos %dx%d %dx%d %dx%d\n",
    (int32_t)tris.vertices[0].pos.x,
    (int32_t)tris.vertices[0].pos.y,
    (int32_t)tris.vertices[1].pos.x,
    (int32_t)tris.vertices[1].pos.y,
    (int32_t)tris.vertices[2].pos.x,
    (int32_t)tris.vertices[2].pos.y
  );
  printf(
    " uv %dx%d %dx%d %dx%d\n",
    (int32_t)tris.vertices[0].uv.x,
    (int32_t)tris.vertices[0].uv.y,
    (int32_t)tris.vertices[1].uv.x,
    (int32_t)tris.vertices[1].uv.y,
    (int32_t)tris.vertices[2].uv.x,
    (int32_t)tris.vertices[2].uv.y
  );
  quads_t q= (quads_t){
    .vertices = {
      tris.vertices[0], tris.vertices[1], tris.vertices[1], tris.vertices[2]
    }
  };

  printf(
    " pos %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)q.vertices[0].pos.x,
    (int32_t)q.vertices[0].pos.y,
    (int32_t)q.vertices[1].pos.x,
    (int32_t)q.vertices[1].pos.y,
    (int32_t)q.vertices[2].pos.x,
    (int32_t)q.vertices[2].pos.y,
    (int32_t)q.vertices[3].pos.x,
    (int32_t)q.vertices[3].pos.y
  );
  printf(
    " uv %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)q.vertices[0].uv.x,
    (int32_t)q.vertices[0].uv.y,
    (int32_t)q.vertices[1].uv.x,
    (int32_t)q.vertices[1].uv.y,
    (int32_t)q.vertices[2].uv.x,
    (int32_t)q.vertices[2].uv.y,
    (int32_t)q.vertices[3].uv.x,
    (int32_t)q.vertices[3].uv.y
  );

  render_push_native_quads(&q, rgba(128,128,128,255), texture_index);
}

void render_push_sprite(vec3_t pos, vec2i_t size, rgba_t color, uint16_t texture_index){
  printf("%s\n", __FUNCTION__);
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
  printf("%s\n", __FUNCTION__);
  vec2i_t tex_size = render_texture_size(texture);
  //toujours des normals sprites ou du vdp2
  //If it is the first one, use a vdp2 surface to render
  //Can use NBG0 and NBG1, with Sprite in between based on priority (to be optimized later)
  printf("Size = %dx%d Texture=%dx%d\n", size.x, size.y, tex_size.x, tex_size.y);
  if ((nb_planes == 0) && (size.x == tex_size.x) && (size.y == tex_size.y)) {
    nb_planes++;
    set_vdp2_texture(texture, pos, size, NBG0);
  } else {
    render_push_2d_tile(pos, vec2i(0, 0), tex_size, size, color, texture);
  }
}
void render_push_2d_tile(vec2i_t pos, vec2i_t uv_offset, vec2i_t uv_size, vec2i_t size, rgba_t color, uint16_t texture_index){
  printf("%s\n", __FUNCTION__);
  //toujours des scaled_sprites
  quads_t q = (quads_t){
		.vertices = {
      {
        .pos = {.x=pos.x, .y=pos.y, .z=0},
        .uv = {.x=uv_offset.x , .y=uv_offset.y},
        .color = color
      },
      {
        .pos = {.x=pos.x + size.x, .y=pos.y, .z=0},
        .uv = {.x=uv_offset.x +  uv_size.x, .y=uv_offset.y},
        .color = color
      },
      {
        .pos = {.x=pos.x + size.x, .y=pos.y + size.y, .z=0},
        .uv = {.x=uv_offset.x + uv_size.x, .y=uv_offset.y + uv_size.y},
        .color = color
      },
      {
        .pos = {.x=pos.x, .y=pos.y + size.y, .z=0},
        .uv = {.x=uv_offset.x , .y=uv_offset.y + uv_size.y},
        .color = color
      },
		}
	};
  printf(
    " pos %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)q.vertices[0].pos.x,
    (int32_t)q.vertices[0].pos.y,
    (int32_t)q.vertices[1].pos.x,
    (int32_t)q.vertices[1].pos.y,
    (int32_t)q.vertices[2].pos.x,
    (int32_t)q.vertices[2].pos.y,
    (int32_t)q.vertices[3].pos.x,
    (int32_t)q.vertices[3].pos.y
  );
  printf(
    " uv %dx%d %dx%d %dx%d %dx%d\n",
    (int32_t)q.vertices[0].uv.x,
    (int32_t)q.vertices[0].uv.y,
    (int32_t)q.vertices[1].uv.x,
    (int32_t)q.vertices[1].uv.y,
    (int32_t)q.vertices[2].uv.x,
    (int32_t)q.vertices[2].uv.y,
    (int32_t)q.vertices[3].uv.x,
    (int32_t)q.vertices[3].uv.y
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
  printf("Need %d kB\n", byte_size/1024);
  printf("Create Texture(%dx%d)\n", width, height);
  rgb1555_t *buffer = (rgb1555_t *)tex_bump(byte_size);

  for (uint32_t i=0; i<width*height; i++) {
    buffer[i] = convert_to_rgb(pixels[i]);
  }

	return allocate_tex(width, height, buffer);
}
vec2i_t render_texture_size(uint16_t texture){
  printf("%s\n", __FUNCTION__);
  return get_tex(texture)->size;
}
void render_texture_replace_pixels(uint16_t texture_index, rgba_t *pixels){
  printf("%s\n", __FUNCTION__);
  render_texture_t *t = get_tex(texture_index);
  for (int i=0; i<t->size.x*t->size.y; i++) {
    t->pixels[i] = convert_to_rgb(pixels[i]);
  }
}
uint16_t render_textures_len(void){
  return tex_length();
}
void render_textures_reset(uint16_t len){
  printf("%s %d\n", __FUNCTION__ , len);
  tex_reset(len);
}
void render_textures_dump(const char *path){printf("%s\n", __FUNCTION__); }