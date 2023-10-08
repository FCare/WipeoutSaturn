#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "texture.h"
#include "image.h"
#include "type.h"
#include "gl.h"

#include "object.h"

static char *temp_path = NULL;

#define SWAP(X) (((X&0xFF)<<8)|(X>>8))

static int currentModel = 0;
static int nb_objects;
static Object** model;

static texture_list_t textures;

static int conversionStep(void) {
  uint16_t format = 0x1; //RGB/palette 16 bits

  if (currentModel < nb_objects) {
    printf("Generating texture for model %s\n", model[currentModel]->name);
    object_draw(model[currentModel++]);
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
		printf("Usage: ./convertModel myfile.cmp myfile.prm\n");
		return -1;
	}
	textures = image_get_compressed_textures(argv[1]);
	Object *models = objects_load(argv[2], &textures);

	int object_index;

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

	if (gl_init(conversionStep) != 0) {
    printf("Error\n");
    exit(-1);
  }

  return 0;
}