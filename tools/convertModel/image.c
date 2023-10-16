#include <stdlib.h>
#include <stdio.h>

#include "image.h"
#include "file.h"
#include "type.h"
#include "lzss.h"
#include "texture.h"

#define SAVE_EXTRACT

#define TIM_TYPE_PALETTED_4_BPP 0x08
#define TIM_TYPE_PALETTED_8_BPP 0x09
#define TIM_TYPE_TRUE_COLOR_16_BPP 0x02

typedef struct {
	uint32_t len;
	uint8_t *entries[];
} cmp_t;

static inline rgba_t tim_16bit_to_rgba(uint16_t c, bool transparent_bit) {
	return rgba(
		((c >>  0) & 0x1f) << 3,
		((c >>  5) & 0x1f) << 3,
		((c >> 10) & 0x1f) << 3,
		(c == 0
			? 0x00
			: transparent_bit && (c & 0x7fff) == 0 ? 0x00 : 0xff
		)
	);
}

static image_t *image_alloc(uint32_t width, uint32_t height) {
	image_t *image = malloc(sizeof(image_t) + width * height * sizeof(rgba_t));
	image->width = width;
	image->height = height;
	image->pixels = (rgba_t *)(((uint8_t *)image) + sizeof(image_t));
	return image;
}

static image_t *image_load_from_bytes(uint8_t *bytes, bool transparent) {
	uint32_t p = 0;

	uint32_t magic = get_i32_le(bytes, &p);
	uint32_t type = get_i32_le(bytes, &p);
	rgba_t palette[256];

	if (
		type == TIM_TYPE_PALETTED_4_BPP ||
		type == TIM_TYPE_PALETTED_8_BPP
	) {
		uint32_t header_length = get_i32_le(bytes, &p);
		uint16_t palette_x = get_i16_le(bytes, &p);
		uint16_t palette_y = get_i16_le(bytes, &p);
		uint16_t palette_colors = get_i16_le(bytes, &p);
		uint16_t palettes = get_i16_le(bytes, &p);
		for (int i = 0; i < palette_colors; i++) {
			palette[i] = tim_16bit_to_rgba(get_u16_le(bytes, &p), transparent);
		}
	}

	uint32_t data_size = get_i32_le(bytes, &p);

	int32_t pixels_per_16bit = 1;
	if (type == TIM_TYPE_PALETTED_8_BPP) {
		pixels_per_16bit = 2;
	}
	else if (type == TIM_TYPE_PALETTED_4_BPP) {
		pixels_per_16bit = 4;
	}

	uint16_t skip_x = get_i16_le(bytes, &p);
	uint16_t skip_y = get_i16_le(bytes, &p);
	uint16_t entries_per_row  = get_i16_le(bytes, &p);
	uint16_t rows = get_i16_le(bytes, &p);

	int32_t width = entries_per_row * pixels_per_16bit;
	int32_t height = rows;
	int32_t entries = entries_per_row * rows;

	image_t *image = image_alloc(width, height);
	int32_t pixel_pos = 0;

	if (type == TIM_TYPE_TRUE_COLOR_16_BPP) {
		for (int i = 0; i < entries; i++) {
			image->pixels[pixel_pos++] = tim_16bit_to_rgba(get_u16_le(bytes, &p), transparent);
		}
	}
	else if (type == TIM_TYPE_PALETTED_8_BPP) {
		for (int i = 0; i < entries; i++) {
			int32_t palette_pos = get_i16_le(bytes, &p);
			image->pixels[pixel_pos++] = palette[(palette_pos >> 0) & 0xff];
			image->pixels[pixel_pos++] = palette[(palette_pos >> 8) & 0xff];
		}
	}
	else if (type == TIM_TYPE_PALETTED_4_BPP) {
		for (int i = 0; i < entries; i++) {
			int32_t palette_pos = get_i16_le(bytes, &p);
			image->pixels[pixel_pos++] = palette[(palette_pos >>  0) & 0xf];
			image->pixels[pixel_pos++] = palette[(palette_pos >>  4) & 0xf];
			image->pixels[pixel_pos++] = palette[(palette_pos >>  8) & 0xf];
			image->pixels[pixel_pos++] = palette[(palette_pos >> 12) & 0xf];
		}
	}

	return image;
}

static cmp_t *image_load_compressed(char *name) {
	printf("load cmp %s\n", name);
	uint32_t compressed_size;
	uint8_t *compressed_bytes = file_load(name, &compressed_size);

	uint32_t p = 0;
	int32_t decompressed_size = 0;
	int32_t image_count = get_i32_le(compressed_bytes, &p);

	// Calculate the total uncompressed size
	for (int i = 0; i < image_count; i++) {
		decompressed_size += get_i32_le(compressed_bytes, &p);
	}

	uint32_t struct_size = sizeof(cmp_t) + sizeof(uint8_t *) * image_count;
	cmp_t *cmp = malloc(struct_size + decompressed_size);
	cmp->len = image_count;

	uint8_t *decompressed_bytes = ((uint8_t *)cmp) + struct_size;

	// Rewind and load all offsets
	p = 4;
	uint32_t offset = 0;
	for (int i = 0; i < image_count; i++) {
		cmp->entries[i] = decompressed_bytes + offset;
		offset += get_i32_le(compressed_bytes, &p);
	}

	lzss_decompress(compressed_bytes + p, decompressed_bytes);
	free(compressed_bytes);

	return cmp;
}

texture_list_t image_get_compressed_textures(char *name) {
	cmp_t *cmp = image_load_compressed(name);
	texture_list_t list = {.len = cmp->len};
	int nbPalette = 0;

  list.texture = malloc(cmp->len* sizeof(texture_t));
	printf("Comp len = %d\n", cmp->len);
	for (int i = 0; i < cmp->len; i++) {
		int32_t width, height;
		image_t *image = image_load_from_bytes(cmp->entries[i], false);

		// char png_name[1024] = {0};
		// sprintf(png_name, "%s_%d.png", name, i);
		// printf("save as %s\n", png_name);
		// stbi_write_png(png_name, image->width, image->height, 4, image->pixels, 0);

		list.texture[i] = texture_create(image->width, image->height, image->pixels);
		list.texture[i]->id = i;
		if (list.texture[i]->format != COLOR_BANK_RGB)
			list.texture[i]->palette.index_in_file = nbPalette++;
		// free(image);

#ifdef SAVE_EXTRACT
	char png_name[1024] = {0};
	sprintf(png_name, "origin_%d.png", i);
	stbi_write_png(png_name, image->width, image->height, 4, image->pixels, 0);
#endif
	}
	printf("Found %d palettes\n", nbPalette);
	free(cmp);
	return list;
}