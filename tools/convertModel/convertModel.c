#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "texture.h"
#include "image.h"
#include "type.h"


#include "object.h"

static char *temp_path = NULL;

uint16_t texture_from_list(texture_list_t tl, uint16_t index) {
	error_if(index >= tl.len, "Texture %d not in list of len %d", index, tl.len);
	return tl.start + index;
}

#define SWAP(X) (((X&0xFF)<<8)|(X>>8))

int main(int argc, char *argv[]) {
  if (argc != 3) {
		printf("Usage: ./convertModel myfile.cmp myfile.prm\n");
		return -1;
	}
	texture_list_t textures = image_get_compressed_textures(argv[1]);
	Object *models = objects_load(argv[2], textures);

	int object_index;
	int nb_objects;
	Object** model;

	nb_objects = 0;

	Object *current = models;

	while(current != NULL) {
		nb_objects++;
		current = current->next;
	}

	model = malloc(sizeof(Object*)*nb_objects);

	for (object_index = 0; object_index < nb_objects && models ; object_index++) {
		model[object_index] = models;
		models = models->next;
	}
	printf("Found %d models\n", nb_objects);

	gl_init();

  uint16_t format = 0x1; //RGB/palette 16 bits

	for (int i=0; i<nb_objects; i++) {
		printf("Generating texture for model %s\n", model[i]->name);
		object_draw(model[i]);
	}

	// palette_length = 0;
	// for (int i = 0; i < cmp->len; i++) {
	// 	int32_t width, height;
	// 	image_t *image = image_load_from_bytes(cmp->entries[i], false);
	//
  //   if (i < UI_SIZE_MAX) {
  //     //fonts
	// 		texture_t out;
  //     char dir[30];
  //     struct stat st = {0};
  //     snprintf(dir, 30, "./output");
  //     if (stat(dir, &st) == -1) {
  //       mkdir(dir, 0777);
  //     }
	// 		char png_name[1024] = {0};
	// 		sprintf(png_name, "./fonts/fonts_%d.stf", char_set[i].height);
	// 		printf("extract %s\n", png_name);
	// 		FILE *f = fopen(png_name, "w+");
	// 		uint16_t offset = (sizeof(texture_t) + 0x7)&~0x7; //offset address shall start on an aligned address to 0x8
	// 		uint16_t current = 0; //offset address shall start on an aligned address to 0x8
	// 		out.format = format;
	// 		out.nbQuads = 38;
	//
	// 		uint16_t format_s= SWAP(out.format);
	// 		uint16_t nbQuads_s= SWAP(out.nbQuads);
	// 		fwrite(&format_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	// 		fwrite(&nbQuads_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	//
  //     for (int j=0; j<38; j++) {
	// 			character_t *ch = &out.character[j];
	// 			ch->width = char_set[i].glyphs[j].width;
	// 			ch->stride = (ch->width + 0x7) &~0x7; //align width to 8
	// 			ch->height = char_set[i].height;
	// 			ch->offset = offset;
	//
	// 			uint16_t width_s= SWAP(ch->width);
	// 			uint16_t stride_s= SWAP(ch->stride);
	// 			uint16_t height_s= SWAP(ch->height);
	// 			uint16_t offset_s= SWAP(ch->offset);
	//
	// 			fwrite(&width_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	// 			fwrite(&stride_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	// 			fwrite(&height_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	// 			fwrite(&offset_s, 1, sizeof(uint16_t), f); current +=sizeof(uint16_t);
	// 			offset += ch->stride*ch->height*sizeof(rgb1555_t);
	// 		}
	// 		//Padding
	// 		uint8_t zero = 0;
	// 		while (current < out.character[0].offset) {
	// 			fwrite(&zero, 1, sizeof(uint8_t), f);
	// 			current +=sizeof(uint8_t);
	// 		}
	//
	// 		//write all pixels
	// 		for (int j=0; j<38; j++) {
	// 			for (int k = char_set[i].glyphs[j].offset.y;
	// 				k<(char_set[i].glyphs[j].offset.y+char_set[i].height);
	// 				k++)
	// 				{
	// 					rgba_t* src =  &image->pixels[k*image->width+char_set[i].glyphs[j].offset.x];
	// 					for (int l = 0; l<char_set[i].glyphs[j].width; l++) {
  //           	rgb1555_t pix = SWAP(convert_to_rgb(src[l]));
	// 						updatePalette(pix);
	// 						fwrite(&pix, 1, sizeof(rgb1555_t), f);
  //         	}
	// 					rgb1555_t pix_zero = 0;
  //         	for (int l = char_set[i].glyphs[j].width; l<out.character[j].stride; l++) {
  //           	fwrite(&pix_zero, 1, sizeof(rgb1555_t), f);
  //         	}
  //       }
  //     }
	// 		fclose(f);
  //   }
	//
	//
	// 	// char png_name[1024] = {0};
	// 	// sprintf(png_name, "%s.%d.png", argv[1], i);
  //   // printf("extract %s\n", png_name);
	// 	// stbi_write_png(png_name, image->width, image->height, 4, image->pixels, 0);
	//
	// 	free(image);
	// }
	// printf("Palette is %d\n", palette_length);

	// free(cmp);
  return 0;
}