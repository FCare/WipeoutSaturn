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

#define MESH_FLAG       0x1
#define POLYGON_FLAG    0x2

extern int read_png_file(char *filename, texture_t *tex);

static texture_t inputTexture;

static model modelOut;
//up to 2048 faces per geometry
static int uv_index[32][2048*4];
static float uv[32][2048*2];

static int currentGeo;
static int currentFace;

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

  currentFace++;

  gl_generate_texture_from_quad(&out, &quad, &inputTexture);
  // uint16_t format = 0x1; //RGB/palette 16 bits
  //
  // if (currentModel < nb_objects) {
  //   LOGD("Generating texture for model %s\n", model[currentModel]->name);
  //   object_draw(model[currentModel++]);
  //   return 1;
  // }
  return 1;
}

static int savingStep(void) {
  LOGD("saving step\n");

  free(inputTexture.pixels);
  return 0;
}

int
main(int argc, char **argv)
{
    xmlDoc         *document;
    xmlNode        *root, *group, *shape;
    char           *filename;
    char           *texturefilename;

    uint32_t vertexOut[2048*3];
    uint32_t normalOut[2048*3];
    face faceOut[32][2048];

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

      modelOut.vertexNb = 0;
      char* DEFIndex = xmlGetProp(coordNode, "DEF");
      if (DEFIndex != NULL) {
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

      int nbNormals = 0;
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
        LOGD("Not a normal per vertex\n");
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
          if (component != 3) faceOut[modelOut.nbGeometry][nbColor/4].RGB |= (col<<(component*5));
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
