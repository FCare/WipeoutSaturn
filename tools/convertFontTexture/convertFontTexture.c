#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef unsigned char uint8_t;
typedef uint8_t bool;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef uint16_t rgb1555_t;
typedef unsigned int uint32_t;

typedef struct {
	uint32_t len;
	uint8_t *entries[];
} cmp_t;

typedef struct rgba_t {
	uint8_t r, g, b, a;
} rgba_t;

#define rgba(R, G, B, A) ((rgba_t){.r = R, .g = G, .b = B, .a = A})

typedef struct {
	uint32_t width;
	uint32_t height;
	rgba_t *pixels;
} image_t;

static rgb1555_t palette[256];
static uint16_t palette_length;

#define true 1
#define false 0

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define die(...) \
	printf("Abort at " TOSTRING(__FILE__) " line " TOSTRING(__LINE__) ": " __VA_ARGS__); \
	printf("\n"); \
	exit(1)

#define error_if(TEST, ...) \
	if (TEST) { \
		die(__VA_ARGS__); \
	}

static char *temp_path = NULL;


#define TIM_TYPE_PALETTED_4_BPP 0x08
#define TIM_TYPE_PALETTED_8_BPP 0x09
#define TIM_TYPE_TRUE_COLOR_16_BPP 0x02

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

image_t *image_alloc(uint32_t width, uint32_t height) {
	image_t *image = malloc(sizeof(image_t) + width * height * sizeof(rgba_t));
	image->width = width;
	image->height = height;
	image->pixels = (rgba_t *)(((uint8_t *)image) + sizeof(image_t));
	return image;
}

static inline uint8_t get_u8(uint8_t *bytes, uint32_t *p) {
	return bytes[(*p)++];
}

static inline uint16_t get_u16(uint8_t *bytes, uint32_t *p) {
	uint16_t v = 0;
	v |= bytes[(*p)++] << 8;
	v |= bytes[(*p)++] << 0;
	return v;
}

static inline uint32_t get_u32(uint8_t *bytes, uint32_t *p) {
	uint32_t v = 0;
	v |= bytes[(*p)++] << 24;
	v |= bytes[(*p)++] << 16;
	v |= bytes[(*p)++] <<  8;
	v |= bytes[(*p)++] <<  0;
	return v;
}

static inline uint16_t get_u16_le(uint8_t *bytes, uint32_t *p) {
	uint16_t v = 0;
	v |= bytes[(*p)++] << 0;
	v |= bytes[(*p)++] << 8;
	return v;
}

static inline uint32_t get_u32_le(uint8_t *bytes, uint32_t *p) {
	uint32_t v = 0;
	v |= bytes[(*p)++] <<  0;
	v |= bytes[(*p)++] <<  8;
	v |= bytes[(*p)++] << 16;
	v |= bytes[(*p)++] << 24;
	return v;
}

#define get_i8(BYTES, P) ((int8_t)get_u8(BYTES, P))
#define get_i16(BYTES, P) ((int16_t)get_u16(BYTES, P))
#define get_i16_le(BYTES, P) ((int16_t)get_u16_le(BYTES, P))
#define get_i32(BYTES, P) ((int32_t)get_u32(BYTES, P))
#define get_i32_le(BYTES, P) ((int32_t)get_u32_le(BYTES, P))

image_t *image_load_from_bytes(uint8_t *bytes, bool transparent) {
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


#define LZSS_INDEX_BIT_COUNT  13
#define LZSS_LENGTH_BIT_COUNT 4
#define LZSS_WINDOW_SIZE      (1 << LZSS_INDEX_BIT_COUNT)
#define LZSS_BREAK_EVEN       ((1 + LZSS_INDEX_BIT_COUNT + LZSS_LENGTH_BIT_COUNT) / 9)
#define LZSS_END_OF_STREAM    0
#define LZSS_MOD_WINDOW(a)    ((a) & (LZSS_WINDOW_SIZE - 1))

static void lzss_decompress(uint8_t *in_data, uint8_t *out_data) {
	int16_t i;
	int16_t current_position;
	uint8_t cc;
	int16_t match_length;
	int16_t match_position;
	uint32_t mask;
	uint32_t return_value;
	uint8_t in_bfile_mask;
	int16_t in_bfile_rack;
	int16_t value;
	uint8_t window[LZSS_WINDOW_SIZE];

	in_bfile_rack = 0;
	in_bfile_mask = 0x80;

	current_position = 1;
	while (true) {
		if (in_bfile_mask == 0x80) {
			in_bfile_rack = (int16_t) * in_data++;
		}

		value = in_bfile_rack & in_bfile_mask;
		in_bfile_mask >>= 1;
		if (in_bfile_mask == 0) {
			in_bfile_mask = 0x80;
		}

		if (value) {
			mask = 1L << (8 - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			cc = (uint8_t) return_value;
			*out_data++ = cc;
			window[ current_position ] = cc;
			current_position = LZSS_MOD_WINDOW(current_position + 1);
		}
		else {
			mask = 1L << (LZSS_INDEX_BIT_COUNT - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			match_position = (int16_t) return_value;

			if (match_position == LZSS_END_OF_STREAM) {
				break;
			}

			mask = 1L << (LZSS_LENGTH_BIT_COUNT - 1);
			return_value = 0;
			while (mask != 0) {
				if (in_bfile_mask == 0x80) {
					in_bfile_rack = (int16_t) * in_data++;
				}

				if (in_bfile_rack & in_bfile_mask) {
					return_value |= mask;
				}
				mask >>= 1;
				in_bfile_mask >>= 1;

				if (in_bfile_mask == 0) {
					in_bfile_mask = 0x80;
				}
			}
			match_length = (int16_t) return_value;

			match_length += LZSS_BREAK_EVEN;

			for (i = 0 ; i <= match_length ; i++) {
				cc = window[LZSS_MOD_WINDOW(match_position + i)];
				*out_data++ = cc;
				window[current_position] = cc;
				current_position = LZSS_MOD_WINDOW(current_position + 1);
			}
		}
	}
}

static uint8_t *file_load(const char *path, uint32_t *bytes_read) {
	FILE *f = fopen(path, "rb");
	error_if(!f, "Could not open file for reading: %s", path);

	fseek(f, 0, SEEK_END);
	int32_t size = ftell(f);
	if (size <= 0) {
		fclose(f);
		return NULL;
	}
	fseek(f, 0, SEEK_SET);

	uint8_t *bytes = malloc(size);
	if (!bytes) {
		fclose(f);
		return NULL;
	}

	*bytes_read = fread(bytes, 1, size, f);
	fclose(f);

	error_if(*bytes_read != size, "Could not read file: %s", path);
	return bytes;
}

static uint8_t *platform_load_asset(const char *name, uint32_t *bytes_read) {
	return file_load(name, bytes_read);
}

static cmp_t *image_load_compressed(char *name) {
	printf("load cmp %s\n", name);
	uint32_t compressed_size;
	uint8_t *compressed_bytes = platform_load_asset(name, &compressed_size);

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

static rgb1555_t RGB888_RGB1555(uint8_t msb, uint8_t r, uint8_t g, uint8_t b) {
  return (rgb1555_t)(((msb&0x1)<<15) | ((r>>0x3)<<10) | ((g>>0x3)<<5) | (b>>0x3));
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

typedef struct {
  uint16_t x,y;
} vec2i_t;

typedef struct {
	vec2i_t offset;
	uint16_t width;
} glyph_t;

typedef struct {
	uint16_t height;
	glyph_t glyphs[38];
} char_set_t;

typedef enum {
	UI_SIZE_16 = 0,
	UI_SIZE_12,
	UI_SIZE_8,
	UI_SIZE_MAX
} ui_text_size_t;

char_set_t char_set[UI_SIZE_MAX] = {
	[UI_SIZE_16] = {
		.height = 16,
		.glyphs = {
			{{  0,   0}, 25}, {{ 25,   0}, 24}, {{ 49,   0}, 17}, {{ 66,   0}, 24}, {{ 90,   0}, 24}, {{114,   0}, 17}, {{131,   0}, 25}, {{156,   0}, 18},
			{{174,   0},  7}, {{181,   0}, 17}, {{  0,  16}, 17}, {{ 17,  16}, 17}, {{ 34,  16}, 28}, {{ 62,  16}, 17}, {{ 79,  16}, 24}, {{103,  16}, 24},
			{{127,  16}, 26}, {{153,  16}, 24}, {{177,  16}, 18}, {{195,  16}, 17}, {{  0,  32}, 17}, {{ 17,  32}, 17}, {{ 34,  32}, 29}, {{ 63,  32}, 24},
			{{ 87,  32}, 17}, {{104,  32}, 18}, {{122,  32}, 24}, {{146,  32}, 10}, {{156,  32}, 18}, {{174,  32}, 17}, {{191,  32}, 18}, {{  0,  48}, 18},
			{{ 18,  48}, 18}, {{ 36,  48}, 18}, {{ 54,  48}, 22}, {{ 76,  48}, 25}, {{101,  48},  7}, {{108,  48},  7}
		}
	},
	[UI_SIZE_12] = {
		.height = 12,
		.glyphs = {
			{{  0,   0}, 19}, {{ 19,   0}, 19}, {{ 38,   0}, 14}, {{ 52,   0}, 19}, {{ 71,   0}, 19}, {{ 90,   0}, 13}, {{103,   0}, 19}, {{122,   0}, 14},
			{{136,   0},  6}, {{142,   0}, 13}, {{155,   0}, 14}, {{169,   0}, 14}, {{  0,  12}, 22}, {{ 22,  12}, 14}, {{ 36,  12}, 19}, {{ 55,  12}, 18},
			{{ 73,  12}, 20}, {{ 93,  12}, 19}, {{112,  12}, 15}, {{127,  12}, 14}, {{141,  12}, 13}, {{154,  12}, 13}, {{167,  12}, 22}, {{  0,  24}, 19},
			{{ 19,  24}, 13}, {{ 32,  24}, 14}, {{ 46,  24}, 19}, {{ 65,  24},  8}, {{ 73,  24}, 15}, {{ 88,  24}, 13}, {{101,  24}, 14}, {{115,  24}, 15},
			{{130,  24}, 14}, {{144,  24}, 15}, {{159,  24}, 18}, {{177,  24}, 19}, {{196,  24},  5}, {{201,  24},  5}
		}
	},
	[UI_SIZE_8] = {
		.height = 8,
		.glyphs = {
			{{  0,   0}, 13}, {{ 13,   0}, 13}, {{ 26,   0}, 10}, {{ 36,   0}, 13}, {{ 49,   0}, 13}, {{ 62,   0},  9}, {{ 71,   0}, 13}, {{ 84,   0}, 10},
			{{ 94,   0},  4}, {{ 98,   0},  9}, {{107,   0}, 10}, {{117,   0}, 10}, {{127,   0}, 16}, {{143,   0}, 10}, {{153,   0}, 13}, {{166,   0}, 13},
			{{179,   0}, 14}, {{  0,   8}, 13}, {{ 13,   8}, 10}, {{ 23,   8},  9}, {{ 32,   8},  9}, {{ 41,   8},  9}, {{ 50,   8}, 16}, {{ 66,   8}, 14},
			{{ 80,   8},  9}, {{ 89,   8}, 10}, {{ 99,   8}, 13}, {{112,   8},  6}, {{118,   8}, 11}, {{129,   8}, 10}, {{139,   8}, 10}, {{149,   8}, 11},
			{{160,   8}, 10}, {{170,   8}, 10}, {{180,   8}, 12}, {{192,   8}, 14}, {{206,   8},  4}, {{210,   8},  4}
		}
	},
};

static const char letter[38] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5','6','7','8','9',':','.'
};

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t stride;
	uint16_t offset;
} character_t;

typedef struct {
	uint16_t format;
	uint16_t nbQuads;
	character_t character[38];
} texture_t;

#define SWAP(X) (((X&0xFF)<<8)|(X>>8))

void updatePalette(rgb1555_t pix) {
	for (int i=0; i< palette_length; i++) {
		if (palette[i] == pix) return;
	}
	if (palette_length >= 256) {
		palette_length++;
	} else {
		palette[palette_length++] = pix;
	}
}

int main(int argc, char *argv[]) {
  if (argc != 2) return -1;
  cmp_t *cmp = image_load_compressed(argv[1]);

  uint16_t format = 0x1; //RGB/palette 16 bits

	palette_length = 0;
	for (int i = 0; i < cmp->len; i++) {
		int32_t width, height;
		image_t *image = image_load_from_bytes(cmp->entries[i], false);

    if (i < UI_SIZE_MAX) {
      //fonts
			texture_t out;
      char dir[30];
      struct stat st = {0};
      snprintf(dir, 30, "./output");
      if (stat(dir, &st) == -1) {
        mkdir(dir, 0777);
      }
			char png_name[1024] = {0};
			sprintf(png_name, "./fonts/fonts_%d.stf", char_set[i].height);
			printf("extract %s\n", png_name);
			FILE *f = fopen(png_name, "w+");
			uint16_t offset = (sizeof(texture_t) + 0x7)&~0x7; //offset address shall start on an aligned address to 0x8
			uint16_t current = 0; //offset address shall start on an aligned address to 0x8
			out.format = format;
			out.nbQuads = 38;

			uint16_t format_s= SWAP(out.format);
			uint16_t nbQuads_s= SWAP(out.nbQuads);
			fwrite(&format_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
			fwrite(&nbQuads_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);

      for (int j=0; j<38; j++) {
				character_t *ch = &out.character[j];
				ch->width = char_set[i].glyphs[j].width;
				ch->stride = (ch->width + 0x7) &~0x7; //align width to 8
				ch->height = char_set[i].height;
				ch->offset = offset;

				uint16_t width_s= SWAP(ch->width);
				uint16_t stride_s= SWAP(ch->stride);
				uint16_t height_s= SWAP(ch->height);
				uint16_t offset_s= SWAP(ch->offset);

				fwrite(&width_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
				fwrite(&stride_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
				fwrite(&height_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
				fwrite(&offset_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
				offset += ch->stride*ch->height*sizeof(rgb1555_t);
			}
			//Padding
			uint8_t zero = 0;
			while (current < out.character[0].offset) {
				fwrite(&zero, 1, sizeof(uint8_t), f);
				current +=sizeof(uint8_t);
			}

			//write all pixels
			for (int j=0; j<38; j++) {
				for (int k = char_set[i].glyphs[j].offset.y;
					k<(char_set[i].glyphs[j].offset.y+char_set[i].height);
					k++)
					{
						rgba_t* src =  &image->pixels[k*image->width+char_set[i].glyphs[j].offset.x];
						for (int l = 0; l<char_set[i].glyphs[j].width; l++) {
            	rgb1555_t pix = SWAP(convert_to_rgb(src[l]));
							updatePalette(pix);
							fwrite(&pix, 1, sizeof(rgb1555_t), f);
          	}
						rgb1555_t pix_zero = 0;
          	for (int l = char_set[i].glyphs[j].width; l<out.character[j].stride; l++) {
            	fwrite(&pix_zero, 1, sizeof(rgb1555_t), f);
          	}
        }
      }
			fclose(f);
    }


		// char png_name[1024] = {0};
		// sprintf(png_name, "%s.%d.png", argv[1], i);
    // printf("extract %s\n", png_name);
		// stbi_write_png(png_name, image->width, image->height, 4, image->pixels, 0);

		free(image);
	}
	printf("Palette is %d\n", palette_length);

	free(cmp);
  return 0;
}