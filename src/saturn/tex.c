#include "tex.h"
#include "map.h"

#define TEXTURES_MAX 2048

static render_texture_t textures[TEXTURES_MAX];

static uint8_t* tex = (uint8_t*)MAP_CS1(MEM_HUNK_BYTES); //2MB on RAM cartridge, after meme hunk
static uint32_t tex_len = 0;

static uint16_t textures_len = 0;

static void *tex_bump(uint32_t size) {
	error_if(((tex_len+0x3)&~0x3) + size >= MEM_HUNK_BYTES, "Failed to allocate %d bytes (%d)", size, tex_len);
	tex_len = (tex_len+0x3)&~0x3;
	uint8_t *p = &tex[tex_len];
	tex_len += size;
	return p;
}

void tex_reset(uint16_t len) {
	LOGD("Tex_reset %d\n", len);
  if (textures_len == len) {
		LOGD("Tex_reset no need\n");
		return;
	}
	tex_len = 0;
	if (len != 0) {
		error_if(((len > textures_len) || (len > TEXTURES_MAX)), "Invalid tex reset");
		render_texture_t * last_texture = &textures[len-1];
		rgb1555_t *new_end = (rgb1555_t *)&last_texture->pixels[last_texture->size.x*last_texture->size.y];
		tex_len = (uint32_t)new_end - (uint32_t)&tex[0];
		LOGD("Tex len now id %x\n", tex_len);
	}
	textures_len = len;
	LOGD("Tex reset to %d\n", textures_len);
}

uint16_t allocate_tex(uint32_t width, uint32_t height, uint32_t size) {
		error_if(textures_len == TEXTURES_MAX, "Not enough texture available");
		rgb1555_t *buffer = (rgb1555_t *)tex_bump(size);
    uint16_t texture_index = textures_len;
    textures[textures_len++] = (render_texture_t){.size = vec2i(width, height), .pixels = buffer};
		LOGD("Allocate tex %d\n", texture_index);
    return texture_index;
}

render_texture_t* get_tex(uint16_t texture) {
    error_if(texture >= textures_len, "Invalid texture %d vs %d", texture,textures_len);
    return &textures[texture];
}

uint16_t create_sub_texture(uint32_t offset, uint32_t width, uint32_t height, uint16_t parent) {
	error_if(textures_len == TEXTURES_MAX, "Not enough texture available");
	uint16_t texture_index = textures_len;
	textures[textures_len++] = (render_texture_t){.size = vec2i(width, height), .pixels = &(get_tex(parent)->pixels[offset/sizeof(rgb1555_t)])};
	LOGD("Allocate sub tex %d\n", texture_index);
	return texture_index;
}

uint16_t tex_length(void) {
	LOGD("return tex length %d\n", textures_len);
  return textures_len;
}
