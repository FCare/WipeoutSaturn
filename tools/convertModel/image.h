#ifndef __IMAGE_H__
#define __IMAGE_H__

#include "texture.h"
#include "type.h"

typedef struct {
	uint32_t width;
	uint32_t height;
	rgba_t *pixels;
} image_t;

extern texture_list_t image_get_compressed_textures(char *name);

#endif