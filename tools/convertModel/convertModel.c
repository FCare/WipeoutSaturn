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
static char *outputObject;
static char *outputTexture;

static int conversionStep(void) {
  // uint16_t format = 0x1; //RGB/palette 16 bits
  //
  // if (currentModel < nb_objects) {
  //   LOGD("Generating texture for model %s\n", model[currentModel]->name);
  //   object_draw(model[currentModel++]);
  //   return 1;
  // }
  return 0;
}

static int savingStep(void) {
  LOGD("We can save the model now\n");
  LOGD("Saving converted models on %s\n", outputObject);
  LOGD("Saving converted texture on %s\n", outputTexture);
  objects_save(outputObject, outputTexture, model , nb_objects, &textures);
  return 0;
}


char *replace_ext(const char *org, const char *new_ext)
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
  int processObject = 0;
  outputObject = NULL;
  outputTexture = NULL;
  if (argc < 2) {
		// LOGD("Usage: ./convertModel myfile.prm myfile.cmp\n");
    LOGD("Usage: ./convertModel myfile.prm\n");
		return -1;
	}
  // if (argc == 3) {
    processObject = 1;
  // }

  nb_objects = 0;
#if 0
  outputTexture = replace_ext(argv[2], ".stf");
	textures = image_get_compressed_textures(argv[1]);
#endif

	if (processObject) {
    outputObject = replace_ext(argv[1], ".smf");
    Object *models = objects_load(argv[1], &textures);

  	int object_index;

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
  	LOGD("Found %d models\n", nb_objects);
  }

	if (gl_init(conversionStep, savingStep) != 0) {
    LOGD("Error\n");
    exit(-1);
  }
  //Never reached
  return 0;
}