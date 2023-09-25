#include "tex.h"
#include "map.h"

#define TEXTURES_MAX 1024

static render_texture_t textures[TEXTURES_MAX];

static uint8_t* tex = (uint8_t*)MAP_CS1(MEM_HUNK_BYTES); //2MB on RAM cartridge, after meme hunk
static uint32_t tex_len = 0;
static uint32_t max_mem = 0;

static int16_t textures_len = 0;

void *tex_bump(uint32_t size) {
	error_if(tex_len + size >= MEM_HUNK_BYTES, "Failed to allocate %d bytes (%d)", size, tex_len);
	uint8_t *p = &tex[tex_len];
	tex_len += size;
  if (max_mem < tex_len) {
    max_mem = tex_len;
    printf("%d Texture Mem => %d kB\n",__LINE__, max_mem/1024);
  }
	return p;
}

void tex_reset(uint16_t len) {
  uint8_t *p = &tex[len];
	uint32_t offset = (uint32_t)p - (uint32_t)tex;
  if (textures_len == len) return;
	error_if(offset > tex_len || offset > MEM_HUNK_BYTES, "Invalid tex reset");
	tex_len = offset;
  if (max_mem < tex_len) {
    max_mem = tex_len;
    printf("%d Texture Mem => %d kB\n",__LINE__, tex_len/1024);
  }
  textures_len = len;
}
uint16_t allocate_tex(uint32_t width, uint32_t height, rgb1555_t *buffer) {
    uint16_t texture_index = textures_len;
    textures[textures_len++] = (render_texture_t){{width, height}, buffer};
    return texture_index;
}

render_texture_t* get_tex(uint16_t texture) {
    error_if(texture >= textures_len, "Invalid texture %d", texture);
    return &textures[texture];
}

uint16_t tex_length(void) {
  return textures_len;
}
