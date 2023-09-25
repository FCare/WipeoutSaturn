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
} vdp1_texture_t;

static vdp1_texture_t textures[2][VDP1_TEXTURES_MAX];

static uint8_t* tex[2];
static uint32_t tex_len[2] = {0};
static uint32_t max_mem[2] = {0};

static uint16_t textures_len[2] = {0};

static uint32_t vdp1_size;

static void *vdp1_tex_bump(uint32_t size, uint8_t id) {
	error_if(tex_len[id] + size >= vdp1_size, "Failed to allocate %d bytes on VDP1 RAM", size);
	uint8_t *p = &tex[id][tex_len[id]];
	tex_len[id] += size;
  if (max_mem[id] < tex_len[id]) {
    max_mem[id] = tex_len[id];
    printf("%d Vdp1 Texture Mem => %d kB\n",__LINE__, max_mem[id]/1024);
  }
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


uint16_t* getVdp1VramAddress(uint16_t texture_index, uint8_t id, quads_t *quad, rgba_t color, vec2i_t *size) {
	render_texture_t* src = get_tex(texture_index);
	int orig_w = quad->vertices[1].uv.x - quad->vertices[0].uv.x;
	int h = quad->vertices[3].uv.y - quad->vertices[0].uv.y;

	//workaround. need to find a fix
	int w = (orig_w+7) & ~0x7; //Align on upper 8

	*size = vec2i(w,h);

	if (w != orig_w) {
		printf("Need extend for %d to %d\n", orig_w, w);
		float ratioX = ((float)w-(float)orig_w)/(float)orig_w;
		//extend the quad to fit the uv
		quad->vertices[1].pos.x += (quad->vertices[1].pos.x - quad->vertices[0].pos.x)*ratioX;
		quad->vertices[2].pos.x += (quad->vertices[2].pos.x - quad->vertices[0].pos.x)*ratioX;
	}

  for (uint16_t i=0; i<textures_len[id]; i++) {
    if ((textures[id][i].index == texture_index) && (isSameUv(textures[id][i].uv, quad)))
		{
      return textures[id][i].pixels;
    }
  }
  //Not found, bump a new one
  uint32_t length = w*h*sizeof(rgb1555_t);
  textures[id][textures_len[id]].index = texture_index;
  textures[id][textures_len[id]].size = *size;
  textures[id][textures_len[id]].pixels = vdp1_tex_bump(length, id);
	rgb1555_t *src_buf = &src->pixels[(int32_t)quad->vertices[0].uv.y * src->size.x + (int32_t)quad->vertices[0].uv.x];
	if (((color.r>>3) == 0)&&((color.g>>3) == 0)&&((color.b>>3) == 0)) {
		for (int i = 0; i<h; i++) {
			memcpy(&textures[id][textures_len[id]].pixels[i*w], &src_buf[i*src->size.x], orig_w*sizeof(rgb1555_t));
			memset(&textures[id][textures_len[id]].pixels[i*w+orig_w], 0, (w-orig_w)*sizeof(rgb1555_t));
		}
	} else {
		for (int i = 0; i<h; i++) {
			for (int j = 0; j<orig_w; j++) {
				rgb1555_t *dst = &textures[id][textures_len[id]].pixels[i*w+j];
				*dst = src_buf[i*src->size.x+j];
				if ((dst->msb != 0) || (dst->r != 0) || (dst->g!=0) || (dst->b != 0)) {
					dst->r *= (color.r/255.0)*2.0;
					dst->g *= (color.g/255.0)*2.0;
					dst->b *= (color.b/255.0)*2.0;
				}
			}
			for (int j = orig_w; j<w; j++) {
				textures[id][textures_len[id]].pixels[i*w+j] = (rgb1555_t){.msb=0,.r=0,.g=0,.b=0};
			}
		}
	}
  return textures[id][textures_len[id]++].pixels;
}

uint16_t canAllocateVdp1(uint16_t texture_index, uint8_t id, quads_t *quad) {
	render_texture_t* src = get_tex(texture_index);
  for (uint16_t i=0; i<textures_len[id]; i++) {
    if ((textures[id][i].index == texture_index) && (isSameUv(textures[id][i].uv, quad)))
		{
			printf("reuseable texture\n");
      return 1;
    }
  }
	//Expect all uv to be square
	int orig_w = quad->vertices[1].uv.x - quad->vertices[0].uv.x;
	int h = quad->vertices[3].uv.y - quad->vertices[0].uv.y;

	int w = (orig_w+7) & ~0x7; //Align on upper 8

  uint32_t size = w*h*sizeof(uint16_t);
  if(tex_len[id] + size >= vdp1_size) {
		printf("Too much texture %d + %d >= %d\n", tex_len[id], size, vdp1_size);
		return 0;
	}

  return 2;
}

void reset_vdp1_pool(uint8_t id) {
  textures_len[id] = 0;
	tex_len[id] = 0;
}
