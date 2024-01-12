/*
   Simple test with libxml2 <http://xmlsoft.org>. It displays the name
   of the root element and the names of all its children (not
   descendents, just children).

   On Debian, compiles with:
   gcc -Wall -o read-xml2 $(xml2-config --cflags) $(xml2-config --libs) \
                    read-xml2.c

*/

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include "type.h"
#include "gl.h"

extern int read_png_file(char *filename, texture_t *tex);

static texture_t inputTexture;

static model modelOut;
//up to 2048 faces per geometry
static int uv_index[MAX_GEOMETRY][MAX_FACE*4];
static float uv[MAX_GEOMETRY][MAX_FACE*2];

static uint32_t vertexOut[MAX_FACE*3];
static uint32_t normalOut[MAX_FACE*3];
static face faceOut[MAX_GEOMETRY][MAX_FACE];

//It means 2048 character max for 32 geometry max;
static character characters[MAX_GEOMETRY][MAX_FACE];

static int currentGeo;
static int currentFace;

static char *outputObject;

static rgb1555_t *palette;

static rgb1555_t* extractPalette(texture_t *tex, int size) {
  if (tex == NULL) {
    printf("Error with palette exctract\n");
    exit(-1);
  }
  rgb1555_t *pal = (rgb1555_t *)calloc(size,sizeof(rgb1555_t));
  int paletteSize = 0;
  for (int i = 0; i<tex->width*tex->height; i++) {
    uint32_t val = (tex->pixels[i*4+3]<<24) | (tex->pixels[i*4+2]<<16) | (tex->pixels[i*4+1]<<8) | (tex->pixels[i*4+0]);
    rgb1555_t pix = rgb155_from_u32(val);
    uint8_t isNewColor = 1;
    for (int p=0; p<paletteSize; p++) {
      if (pix == pal[p]) isNewColor = 0;
    }
    if (isNewColor == 1) {
      if (paletteSize == size) {
        printf("Error - Palette can not be built\n");
        exit(-1);
      }
      pal[paletteSize++] = pix;
    }
  }
  return pal;
}

static xmlNode* getNodeNamed(xmlNode * root, const char * name) {
  if (root->children == NULL) return NULL;
  for (xmlNode *node = root->children; node; node = node->next) {
    if ((node->type == XML_ELEMENT_NODE) && (strcmp(node->name, name)==0)) return node;
    else {
      xmlNode *temp = getNodeNamed(node, name);
      if (temp != NULL) return temp;
    }
  }
  return NULL;
}


static xmlNode* getShapeInGroup(xmlNode *root) {
  if (root == NULL) return NULL;
  for (xmlNode *node = root; node; node = node->next) {
    if ((node->type == XML_ELEMENT_NODE) && (strcmp(node->name, "Shape")==0)) return node;
  }
  return NULL;
}

static void printChild(xmlNode* root) {
  if (root->children == NULL) return;
  for (xmlNode *node = root->children; node; node = node->next) {
    LOGD("%s contains %s\n", root->name, node->name);
  }
}

//Besoin de gerer le USE vs DEF. Pour le moment pas plus besoin que ca...

// NAME 32
// VERTEX SIZE  4
// VERTEX OFFSET 4
// NB_GEOMETRY 4
// NB_SECTORS 4
// {
//   NAME 28
//   FLAG 4
//   FACE SIZE 4
//   FACE OFFSET 4 //Contains color
//   TEXTURE SIZE 4
//   TEXTURE OFFSET 4
//   NORMAL SIZE 4
//   NORMAL OFFSET 4
// }

// FACE
// A 2
// B 2
// C 2
// D 2
// RGB 2
// PADDING 2

//
// TEXTURE
// W 2
// H 2
// FORMAT 1
// PADDING 3
// DATA
//
// VERTEX //fixed16
// X 4
// Y 4
// Z 4
//
// NORMALS //fixed16
// X 4
// Y 4
// Z 4

static int conversionStep(void) {
  if (currentFace == modelOut.geometry[currentGeo].nbFaces) {
    LOGD("Last Geo was %d faces\n", currentFace);
    currentFace = 0;
    currentGeo++;
  }
  //Check if geometry needs conversion, otherwise, skip it
  while(((modelOut.geometry[currentGeo].flag & POLYGON_FLAG) == POLYGON_FLAG) && (currentGeo<modelOut.nbGeometry)){
    LOGD("Geo %d is polygon - bypass\n", currentGeo);
    currentGeo++;
  }
  if (currentGeo == modelOut.nbGeometry) {
    LOGD("All Geo done - Exit\n");
    return 0;
  }
  quads_t quad;
  render_texture_t out;

  quad.uv[0].x = (int16_t) (uv[currentGeo][uv_index[currentGeo][currentFace*4]*2] * inputTexture.width);
  quad.uv[0].y = (int16_t) (1.0 - uv[currentGeo][uv_index[currentGeo][currentFace*4]*2+1] * inputTexture.height);
  quad.uv[1].x = (int16_t) (uv[currentGeo][uv_index[currentGeo][currentFace*4+1]*2] * inputTexture.width);
  quad.uv[1].y = (int16_t) (1.0 - uv[currentGeo][uv_index[currentGeo][currentFace*4+1]*2+1] * inputTexture.height);
  quad.uv[2].x = (int16_t) (uv[currentGeo][uv_index[currentGeo][currentFace*4+2]*2] * inputTexture.width);
  quad.uv[2].y = (int16_t) (1.0 - uv[currentGeo][uv_index[currentGeo][currentFace*4+2]*2+1] * inputTexture.height);
  quad.uv[3].x = (int16_t) (uv[currentGeo][uv_index[currentGeo][currentFace*4+3]*2] * inputTexture.width);
  quad.uv[3].y = (int16_t) (1.0 - uv[currentGeo][uv_index[currentGeo][currentFace*4+3]*2+1] * inputTexture.height);

  //Request OpenGL to render the character
  gl_generate_texture_from_quad(&out, &quad, &inputTexture);
  //save the character as a paletized buffer
  character *c = &characters[currentGeo][currentFace];
  c->width = out.width;
  c->height = out.height;
  c->pixels = (uint32_t*) malloc(out.width*out.height/8*sizeof(uint32_t));
  for (int j=0; j < c->height; j++) {
    for (int i=0; i < c->width; i+=8) {
      uint32_t *val = &c->pixels[(i + j*c->width)/8];
      *val = 0;
      for (int p = 0; p<8; p++) {
        rgb1555_t pix = out.pixels[i+p + j*c->width];
        for(int k = 0; k<16; k++) {
          if (pix == palette[k]) {
            *val |= k<<((7-p)*4); //maybe need to reverse here
            break;
          }
        }
      }
    }
  }
  free(out.pixels);
  currentFace++;
  return 1;
}

static void write_16(uint16_t val, FILE *f) {
	uint16_t tmp = SWAP(val);
	fwrite(&tmp, 2, sizeof(uint8_t), f);
}

static void write_32(uint32_t val, FILE *f) {
	uint32_t tmp = SWAP_32(val);
	fwrite(&tmp, 4, sizeof(uint8_t), f);
}

static int savingStep(void) {
  LOGD("saving step\n");

  long vertexPos = 0;
  long normalPos = 0;
  long facePos[MAX_GEOMETRY] = {0};
  long charTablePos[MAX_GEOMETRY] = {0};
  long charPos[MAX_GEOMETRY][MAX_FACE] = {0};

  FILE *fobj = fopen(outputObject, "wb+");
  fwrite(modelOut.name, 32, sizeof(char), fobj);
  write_32(modelOut.vertexNb, fobj);
  vertexPos = ftell(fobj);
  write_32(0, fobj); //vertexOffset
  normalPos = ftell(fobj);
  write_32(0, fobj); //normalOffset
  write_32(modelOut.nbGeometry, fobj);
  for (int i=0; i<16; i++) {
    write_16(palette[i], fobj);
  }
  for (int i=0; i<modelOut.nbGeometry; i++) {
    write_32(modelOut.geometry[i].flag, fobj);
    write_32(modelOut.geometry[i].nbFaces, fobj);
    facePos[i] = ftell(fobj);
    write_32(0, fobj); //facesoffset
    charTablePos[i] = ftell(fobj);
    write_32(0, fobj); //characterOffset
  }
  //Write vertex
  long pos = ftell(fobj);
  // pos = (pos + BINARY_SECTOR_SIZE) & ~(BINARY_SECTOR_SIZE-1);
  //pack all starting the next sector
  fseek(fobj, vertexPos, SEEK_SET);
  write_32(pos, fobj); //vertexOffset
  fseek(fobj, pos, SEEK_SET);
  for (int i=0; i<modelOut.vertexNb;i++) {
    write_32(vertexOut[i*3], fobj);
    write_32(vertexOut[i*3+1], fobj);
    write_32(vertexOut[i*3+2], fobj);
  }
  //Write normals
  pos = ftell(fobj);
  fseek(fobj, normalPos, SEEK_SET);
  write_32(pos, fobj); //normalOffset
  fseek(fobj, pos, SEEK_SET);
  for (int i=0; i<modelOut.vertexNb;i++) {
    write_32(normalOut[i*3], fobj);
    write_32(normalOut[i*3+1], fobj);
    write_32(normalOut[i*3+2], fobj);
  }
  //Write faces
  for (int i=0; i<modelOut.nbGeometry; i++) {
    pos = ftell(fobj);
    fseek(fobj, facePos[i], SEEK_SET);
    write_32(pos, fobj); //facesoffset
    fseek(fobj, pos, SEEK_SET);
    for (int j=0; j<modelOut.geometry[i].nbFaces; j++) {
      write_32(faceOut[i][j].vertex_id[0], fobj);
      write_32(faceOut[i][j].vertex_id[1], fobj);
      write_32(faceOut[i][j].vertex_id[2], fobj);
      write_32(faceOut[i][j].vertex_id[3], fobj);
      write_32(faceOut[i][j].RGB, fobj);
    }
  }

  //Write characters
  pos = ftell(fobj);
  for (int i=0; i<modelOut.nbGeometry; i++) {
    for (int j=0; j<modelOut.geometry[i].nbFaces; j++) {
      charPos[i][j] = ftell(fobj);
      write_32(characters[i][j].width, fobj);
      write_32(characters[i][j].height, fobj);
      for (int p=0; p<characters[i][j].width*characters[i][j].height/8; p++) {
        write_32(characters[i][j].pixels[p], fobj);
      }
    }
    pos = ftell(fobj);
    fseek(fobj, charTablePos[i], SEEK_SET);
    write_32(pos, fobj); //characteroffset
    fseek(fobj, pos, SEEK_SET);
    for (int j=0; j<modelOut.geometry[i].nbFaces; j++) {
      write_32(charPos[i][j], fobj); //characterOffset
    }
  }

  printf("Wrote %ld bytes\n", pos);
  fclose(fobj);
  free(inputTexture.pixels);
  return 0;
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

static void getNameFromPath(const char *org, char * out, int max) {
  char *tmp = strdup(org);
  char *start = tmp;
  char *ext = strrchr(tmp , '/');
  if (ext) { start = ext+1; }
  ext = strrchr(start, '.');
  if (ext) { *ext = '\0'; }
  snprintf(out, 32, "%s", start);
  free(tmp);
}

int
main(int argc, char **argv)
{
    xmlDoc         *document;
    xmlNode        *root, *group, *shape;
    char           *filename;
    char           *texturefilename;
    int             nbNormals = 0;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s filename.x3D texture.png\n", argv[0]);
        return 1;
    }
    filename = argv[1];
    texturefilename = argv[2];


    if (read_png_file(texturefilename, &inputTexture) < 0) {
      fprintf(stderr, "Texture can not be read\n");
      return 1;
    } else {
      fprintf(stdout, "Texture is valid, size is %dx%d\n", inputTexture.width, inputTexture.height);
    }
    outputObject = replace_ext(filename, ".smf");

    getNameFromPath(filename,modelOut.name, 32);

    printf("Output to %s\n", outputObject);

    palette = extractPalette(&inputTexture, 16);

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);
    // fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    group = getNodeNamed(root, "Group");
    if (group == NULL) {
      fprintf(stdout, "No group found\n");
      free(inputTexture.pixels);
      xmlFreeDoc(document);
      return -1;
    }

    modelOut.nbGeometry = 0;
    modelOut.vertexNb = 0;
    for(shape = group->children; shape = getShapeInGroup(shape); shape = shape->next) {
      LOGD("New Geometry\n");
      geometry *geo = &modelOut.geometry[modelOut.nbGeometry];
      geo->flag = 0;
      xmlNode *material = getNodeNamed(shape, "Material");
      if (material == NULL) {
        fprintf(stdout, "No material found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }
      LOGD("Material name = %s\n", xmlGetProp(material, "DEF"));
      if (strcasestr(xmlGetProp(material, "DEF"), "colored") != NULL)  {
        LOGD("Geo %d is polygon only\n", modelOut.nbGeometry);
        geo->flag |= POLYGON_FLAG;
      }
      if (strcasestr(xmlGetProp(material, "DEF"), "fire") != NULL)     geo->flag |= MESH_FLAG;
      xmlNode *faceset = getNodeNamed(shape, "IndexedFaceSet");
      if (faceset == NULL) {
        fprintf(stdout, "No faceset found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }
      if(strcmp(xmlGetProp(faceset, "normalPerVertex"), "true")!=0) {
        LOGD("No normals - not supported\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }
      if(strcmp(xmlGetProp(faceset, "colorPerVertex"), "false")!=0) {
        LOGD("Colors per vertex - not supported\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      char* texIndex = xmlGetProp(faceset, "texCoordIndex");
      int nb_entry = 0;
      char *str;
      geo->nbFaces = 0;
      while((str=strsep(&texIndex, " ")) != NULL) {
        int val = (int)strtol(str, NULL, 10);
        if (val == -1) {
          if (geo->nbFaces!=0) {
            //Ensure face has 4 vertex
            for (nb_entry; nb_entry <= 4; nb_entry++)
              uv_index[modelOut.nbGeometry][geo->nbFaces*4+nb_entry] = uv_index[modelOut.nbGeometry][geo->nbFaces*4+nb_entry-1];
          }
          geo->nbFaces++;
          nb_entry = 0;
        }
        else if (strlen(str)>0) {
          uv_index[modelOut.nbGeometry][geo->nbFaces*4+nb_entry] = val;
          nb_entry+=1;
        }
      }

      char* coordIndex = xmlGetProp(faceset, "coordIndex");
      nb_entry = 0;
      geo->nbFaces = 0;
      while((str=strsep(&coordIndex, " ")) != NULL) {
        int val = (int)strtol(str, NULL, 10);
        if (val == -1) {
          if (geo->nbFaces!=0) {
            //Ensure face has 4 vertex
            for (nb_entry; nb_entry <= 4; nb_entry++)
              faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry] = faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry-1];
          }
          geo->nbFaces++;
          nb_entry = 0;
        }
        else if (strlen(str)>0) faceOut[modelOut.nbGeometry][geo->nbFaces].vertex_id[nb_entry++] = val;
      }

      LOGD("nbFaces = %d \n", geo->nbFaces);

      xmlNode *coordNode = getNodeNamed(shape, "Coordinate");
      if (coordNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      char* DEFIndex = xmlGetProp(coordNode, "DEF");
      if (DEFIndex != NULL) {
        modelOut.vertexNb = 0;
        char* coordIndex = xmlGetProp(coordNode, "point");
        while((str=strsep(&coordIndex, " ")) != NULL) {
          if (strlen(str)>0) {
            //convert to fix 16.
            uint32_t val = FIX16(strtod(str, NULL));
            vertexOut[modelOut.vertexNb++] = val;
          }
        }
        if ((modelOut.vertexNb%3) != 0) {
          LOGD("Error with vertex number %d\n", modelOut.vertexNb);
          free(inputTexture.pixels);
          xmlFreeDoc(document);
          return -1;
        }
        modelOut.vertexNb = modelOut.vertexNb/3;
        LOGD("nbVertex = %d\n", modelOut.vertexNb);
      }

      xmlNode *normNode = getNodeNamed(shape, "Normal");
      if (normNode == NULL) {
        fprintf(stdout, "No Normal found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      if(xmlGetProp(normNode, "DEF") != NULL) {
        char* coordIndex = xmlGetProp(normNode, "vector");
        while((str=strsep(&coordIndex, " ")) != NULL) {
          if (strlen(str)>0) {
            //convert to fix 16.
            uint32_t val = FIX16(strtod(str, NULL));
            normalOut[nbNormals++] = val;
          }
        }
        if ((nbNormals%3) != 0) {
          LOGD("Error with vertex number %d\n", nbNormals);
          free(inputTexture.pixels);
          xmlFreeDoc(document);
          return -1;
        }
        LOGD("nbNormals = %d\n", nbNormals/3);
      }

      if (nbNormals/3 != modelOut.vertexNb) {
        LOGD("Not a normal per vertex %d %d\n", nbNormals/3, modelOut.vertexNb);
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      xmlNode *texcoordNode = getNodeNamed(shape, "TextureCoordinate");
      if (texcoordNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      int nbUv = 0;
      char* texcoordIndex = xmlGetProp(texcoordNode, "point");
      while((str=strsep(&texcoordIndex, " ")) != NULL) {
        if (strlen(str)>0) {
          uv[modelOut.nbGeometry][nbUv++] = (float)(strtod(str, NULL));
        }
      }
      if ((nbUv%2) != 0) {
        LOGD("Error with uv number %d\n", nbUv);
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }
      LOGD("nbUv = %d\n", nbUv/2);

      xmlNode *colorNode = getNodeNamed(shape, "ColorRGBA");
      int faceId = 0;
      if (colorNode == NULL) {
        fprintf(stdout, "No Coordinate found\n");
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }

      int nbColor = 0;
      char* colorIndex = xmlGetProp(colorNode, "color");
      while((str=strsep(&colorIndex, " ")) != NULL) {
        if (strlen(str)>0) {
          nbColor++;
          uint16_t col = (uint16_t)(strtod(str, NULL) * 32.0 + 0.5);
          int component = nbColor%4;
          if (component != 3) faceOut[modelOut.nbGeometry][nbColor/4].RGB |= (col&0x1F)<<(component*5);
          faceOut[modelOut.nbGeometry][nbColor/4].RGB |= 0x8000;
        }
      }
      if ((nbColor%4) != 0) {
        LOGD("Error with color number %d\n", nbColor);
        free(inputTexture.pixels);
        xmlFreeDoc(document);
        return -1;
      }
      LOGD("nbColor = %d\n", nbColor/4);

      modelOut.nbGeometry++;
    }

    currentGeo = 0;
    currentFace = 0;

    xmlFreeDoc(document);

    gl_init(conversionStep, savingStep);

    return 0;
}
