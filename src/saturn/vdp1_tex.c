#include <yaul.h>
#include "vdp1_tex.h"
#include "map.h"
#include "tex.h"

#define VDP1_TEXTURES_MAX  64

typedef struct {
	vec2i_t size;
  uint16_t index;
	vec2_t uv[4];
	rgb1555_t *pixels;
	uint8_t used;
} vdp1_texture_t;

static vdp1_texture_t textures[2][VDP1_TEXTURES_MAX];

static uint8_t* tex[2];
static uint32_t tex_len[2] = {0};

static uint16_t textures_len[2] = {0};

static uint32_t vdp1_size;

static void *vdp1_tex_bump(uint32_t size, uint8_t id) {
	error_if(tex_len[id] + size >= vdp1_size, "Failed to allocate %d bytes on VDP1 RAM", size);
	uint8_t *p = &tex[id][tex_len[id]];
	tex_len[id] += size;
	return p;
}

void init_vdp1_tex(void) {

	vdp1_vram_partitions_t vdp1_vram_partitions;

	vdp1_vram_partitions_get(&vdp1_vram_partitions);

	vdp1_size = vdp1_vram_partitions.texture_size/2;
  tex[0] = (uint8_t*)vdp1_vram_partitions.texture_base;
  tex[1] = (uint8_t*)(vdp1_vram_partitions.texture_base+vdp1_size);
}

static uint8_t isSameUv(vec2_t *uv, quads_t *q) {
	for (int i = 0; i<4; i++) {
		if (uv[i].x != q->vertices[i].uv.x) return 0;
		if (uv[i].y != q->vertices[i].uv.y) return 0;
	}
	return 1;
}

uint16_t* getVdp1VramAddress(uint16_t texture_index, uint8_t id, quads_t *quad, vec2i_t *size) {
	render_texture_t* src = get_tex(texture_index);
	int orig_w = quad->vertices[1].uv.x - quad->vertices[0].uv.x;
	int h = quad->vertices[3].uv.y - quad->vertices[0].uv.y; //Here uv are simple uint32_t

	assert(orig_w >= 0);
	assert(h >= 0);
	//workaround. need to find a fix
	uint32_t w = (orig_w+7) & ~0x7; //Align on upper 8

	uint32_t length = w*h*sizeof(rgb1555_t);

	*size = vec2i(w,h);

	if (w != orig_w) {
		LOGD("Need extend for %d to %d\n", orig_w, w);
		fix16_t ratioX = fix16_div(FIX16(w - orig_w),FIX16(orig_w));
		//extend the quad to fit the uv
		quad->vertices[1].pos.x += fix16_mul((quad->vertices[1].pos.x - quad->vertices[0].pos.x),ratioX);
		quad->vertices[2].pos.x += fix16_mul((quad->vertices[2].pos.x - quad->vertices[0].pos.x),ratioX);
	}

  for (uint16_t i=0; i<textures_len[id]; i++) {
    if ((textures[id][i].index == texture_index) && isSameUv(textures[id][i].uv, quad))
		{
			textures[id][i].used = 1;
				LOGD("&&&&&&&&&&&&&&& Texture vdp1 %d(0x%x) reused %dx%d => %dx%d:-) :-) :-)\n", i, textures[id][i].pixels, (int32_t)quad->vertices[0].uv.y, (int32_t)quad->vertices[0].uv.x, (int32_t)quad->vertices[3].uv.y, (int32_t)quad->vertices[3].uv.x);
      return textures[id][i].pixels;
    }
  }

  //Not found, bump a new one
	textures[id][textures_len[id]].used = 1;
	for (int i = 0; i<4; i++) {
		textures[id][textures_len[id]].uv[i].x = quad->vertices[i].uv.x;
		textures[id][textures_len[id]].uv[i].y = quad->vertices[i].uv.y;
	}
  textures[id][textures_len[id]].index = texture_index;
  textures[id][textures_len[id]].size = *size;
  textures[id][textures_len[id]].pixels = vdp1_tex_bump(length, id);
	rgb1555_t *src_buf = &src->pixels[(int32_t)quad->vertices[0].uv.y * src->size.x + (int32_t)quad->vertices[0].uv.x];
	LOGD("&&&&&&&&&&&&&&& Texture vdp1 from %d(0x%x) at %dx%d=>%dx%d!!!!!!!!!!!!!!!!ééééééééé\n", textures_len[id], textures[id][textures_len[id]].pixels, (int32_t)quad->vertices[0].uv.y, (int32_t)quad->vertices[0].uv.x, (int32_t)quad->vertices[3].uv.y, (int32_t)quad->vertices[3].uv.x);
	for (int i = 0; i<h; i++) {
		scu_dma_transfer(0, (void *)&textures[id][textures_len[id]].pixels[i*w], &src_buf[i*src->size.x], orig_w*sizeof(rgb1555_t));
		memset(&textures[id][textures_len[id]].pixels[i*w+orig_w], 0, (w-orig_w)*sizeof(rgb1555_t));
		scu_dma_transfer_wait(0);
	}
  return textures[id][textures_len[id]++].pixels;
}

uint16_t canAllocateVdp1(uint16_t texture_index, uint8_t id, quads_t *quad) {
	render_texture_t* src = get_tex(texture_index);
  for (uint16_t i=0; i<textures_len[id]; i++) {
    if ((textures[id][i].index == texture_index) && (isSameUv(textures[id][i].uv, quad)))
		{
			LOGD("reuseable texture\n");
      return 1;
    }
  }
	//Expect all uv to be square
	int orig_w = quad->vertices[1].uv.x - quad->vertices[0].uv.x;
	int h = quad->vertices[3].uv.y - quad->vertices[0].uv.y;

	int w = (orig_w+7) & ~0x7; //Align on upper 8

  uint32_t size = w*h*sizeof(uint16_t);
  if(tex_len[id] + size >= vdp1_size) {
		LOGD("Too much texture %d + %d >= %d\n", tex_len[id], size, vdp1_size);
		return 0;
	}

  return 2;
}

void reset_vdp1_pool(uint8_t id) {
	LOGD("%s\n", __FUNCTION__);
	uint8_t id_reset = 0;
	while((textures[id][id_reset].used == 1) && (id_reset < textures_len[id])) id_reset++;
	for (int i = 0; i<textures_len[id]; i++) {
		textures[id][i].used = 0;
	}
	LOGD("reset[%d] to %d vs %d\n", id, id_reset, textures_len[id]);
	if (textures_len[id] < id_reset){
		textures_len[id] = id_reset;
		if (id_reset != 0) {
			vdp1_texture_t *first = &textures[id][0];
			vdp1_texture_t *last = &textures[id][id_reset];
			tex_len[id] = (uint32_t)(&last->pixels[0] - &(first->pixels[0]));
		} else {
			tex_len[id] = 0;
		}
	}
}

void clear_vdp1_pool(void) {
	for (int id = 0; id < 2; id++){
		for (int i = 0; i<textures_len[id]; i++) {
			textures[id][i].used = 0;
		}
		textures_len[id] = 0;
		tex_len[id] = 0;
	}
}
