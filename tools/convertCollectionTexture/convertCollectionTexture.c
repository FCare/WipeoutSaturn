#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//#define SAVE_EXTRACT

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

#define LOGD printf

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define die(...) \
	LOGD("Abort at " TOSTRING(__FILE__) " line " TOSTRING(__LINE__) ": " __VA_ARGS__); \
	LOGD("\n"); \
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

	printf("Type is %d\n", type);

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
	LOGD("load cmp %s\n", name);
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
	uint16_t width;
	uint16_t height;
	uint16_t offset;
} collection_image_t;

typedef struct {
	uint16_t format;
	uint16_t nbImg;
	rgb1555_t palette[256];
	collection_image_t image[];
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

static char *replace_ext(const char *org, const char *new_ext)
{
    char *ext;
    char *tmp = strdup(org);
    ext = strrchr(tmp , '.');
    if (ext) { *ext = '\0'; }
    size_t new_size = strlen(tmp) + strlen(new_ext) + 1;
    char *new_name = malloc(new_size);
    sprintf(new_name, "%s%s", tmp, new_ext);
    free(tmp);
    return new_name;
}


int main(int argc, char *argv[]) {
	char *outputObject;
	texture_t out;
  if (argc != 2) {
		printf("usage: %s filename\n", argv[0]);
		return -1;
	}
  cmp_t *cmp = image_load_compressed(argv[1]);

	outputObject = replace_ext(argv[1], ".smf");

  uint16_t format = 0x1; //RGB/palette 16 bits

	palette_length = 0;
	image_t **images = malloc(sizeof(image_t*) * cmp->len);
	for (int i = 0; i < cmp->len; i++) {
		images[i] = image_load_from_bytes(cmp->entries[i], false);
  }
	LOGD("extract %s\n", outputObject);
	FILE *f = fopen(outputObject, "w+");

	uint16_t offset = 0; //offset address shall start on an aligned address to 0x8
	out.format = format;
	out.nbImg = cmp->len;

	uint16_t format_s= SWAP(out.format);
	uint16_t nbImg_s= SWAP(out.nbImg);
	fwrite(&format_s, 1, sizeof(uint16_t), f); offset += sizeof(uint16_t);
	fwrite(&nbImg_s, 1, sizeof(uint16_t), f); offset += sizeof(uint16_t);

	palette[0] = 0;
	for (int i=0; i<cmp->len; i++) {
		for (int j=0; j<images[i]->height; j++) {
			rgba_t* src =  &images[i]->pixels[j*images[i]->width];
			for (int l = 0; l<images[i]->width; l++) {
				rgb1555_t pix = SWAP(convert_to_rgb(src[l]));
				updatePalette(pix);
			}
		}
	}
	for (int i=0; i<(sizeof(palette)/sizeof(rgb1555_t)); i++) {
		uint16_t pal_s= SWAP(palette[i]);
		fwrite(&pal_s, 1, sizeof(uint16_t), f);
	}
	offset += sizeof(palette);
	offset += cmp->len * ((sizeof(collection_image_t)+0x7)&~0x7);
	collection_image_t *col = (collection_image_t *)malloc(sizeof(collection_image_t) * cmp->len);
  for (int j=0; j<cmp->len; j++) {
		col[j].width = images[j]->width;
		col[j].height = images[j]->height;
		col[j].offset = offset;

		uint16_t width_s= SWAP(col[j].width);
		uint16_t height_s= SWAP(col[j].height);
		uint16_t offset_s= SWAP(col[j].offset);
		fwrite(&width_s, 1, sizeof(uint16_t), f);
		fwrite(&height_s, 1, sizeof(uint16_t), f);
		fwrite(&offset_s, 1, sizeof(uint16_t), f);
		fseek(f, 2, SEEK_CUR);
		offset += col[j].width*col[j].height*sizeof(rgb1555_t);
	}

	for (int i=0; i<cmp->len; i++) {
		fseek(f, col[i].offset, SEEK_SET);
		for (int j=0; j<images[i]->height; j++) {
			rgba_t* src =  &images[i]->pixels[j*images[i]->width];
			for (int l = 0; l<images[i]->width; l++) {
    		rgb1555_t pix = SWAP(convert_to_rgb(src[l]));
				fwrite(&pix, 1, sizeof(rgb1555_t), f);
  		}
		}
		#ifdef SAVE_EXTRACT
		char png_name[1024] = {0};
		sprintf(png_name, "%s.%d.png", argv[1], i);
		LOGD("extract %s\n", png_name);
		stbi_write_png(png_name, images[i]->width, images[i]->height, 4, images[i]->pixels, 0);
		#endif
	}
	fclose(f);

	// }
	LOGD("Palette is %d\n", palette_length);

	free(col);
	free(images);
	free(cmp);
  return 0;
}